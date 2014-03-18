#include <yandex/contest/invoker/ProcessGroup.hpp>

#include <yandex/contest/invoker/Container.hpp>
#include <yandex/contest/invoker/ContainerError.hpp>
#include <yandex/contest/invoker/Process.hpp>

#include <yandex/contest/system/lxc/Error.hpp>

#include <yandex/contest/detail/IntrusivePointerHelper.hpp>

#include <boost/assert.hpp>

#include <thread>

#include <csignal>

namespace yandex{namespace contest{namespace invoker
{
    YANDEX_CONTEST_INTRUSIVE_PTR_DEFINE(ProcessGroup)

    ProcessGroup::ProcessGroup(const ContainerPointer &container):
        container_(container)
    {
        BOOST_ASSERT(container_);
    }

    ProcessGroupPointer ProcessGroup::create(const ContainerPointer &container)
    {
        ProcessGroupPointer ret(new ProcessGroup(container));
        container->processGroupDefaultSettings().setUpProcessGroup(ret);
        return ret;
    }

    ProcessGroup::~ProcessGroup()
    {
    }

    ProcessPointer ProcessGroup::createProcess(
        const boost::filesystem::path &executable)
    {
        if (processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyStartedError());
        const std::size_t id = task_.processes.size();
        task_.processes.resize(id+1);
        processTask(id).executable = executable;
        return Process::create(ProcessGroupPointer(this), id);
    }

    Pipe ProcessGroup::createPipe()
    {
        if (processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyStartedError());
        return Pipe(task_.pipesNumber++);
    }

    void ProcessGroup::start()
    {
        if (processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyStartedError());
        processGroup_ = container_->execute(task_);
    }

    void ProcessGroup::stop()
    {
        if (!processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
        if (!container_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyTerminatedError());
        container_->stop();
        try
        {
            wait();
        }
        catch (detail::execution::AsyncProcessGroupControlProcessError &e)
        {
            // it's ok (stop() caused it)
        }
        container_.reset();
        result_ = detail::execution::AsyncProcessGroup::Result();
        result_->processGroupResult.completionStatus =
            ProcessGroup::Result::CompletionStatus::STOPPED;
        result_->processResults.resize(task_.processes.size());
        for (Process::Result &pr: result_->processResults)
            pr.completionStatus = Process::Result::CompletionStatus::STOPPED;
    }

    const ProcessGroup::Result &ProcessGroup::synchronizedCall()
    {
        start();
        return wait();
    }

    void ProcessGroup::freeze()
    {
        poll();
        if (!processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
        if (!container_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyTerminatedError());
        try
        {
            // process has not started yet
            while (container_->state() == system::lxc::Lxc::State::STOPPED)
            {
                if (poll())
                {
                    // process has started and terminated while we were sleeping
                    BOOST_THROW_EXCEPTION(
                        ProcessGroupHasAlreadyTerminatedError());
                }
                // FIXME hardcode
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            const system::lxc::Lxc::State state = container_->state();
            if (state != system::lxc::Lxc::State::RUNNING)
                BOOST_THROW_EXCEPTION(
                    ContainerIllegalStateError() <<
                    system::lxc::IllegalStateError::state(state));
            container_->freeze();
        }
        catch (system::lxc::UtilityError &e)
        {
            if (poll())
                BOOST_THROW_EXCEPTION(
                    ProcessGroupHasAlreadyTerminatedError());
            else
                throw;
        }
    }

    void ProcessGroup::unfreeze()
    {
        poll();
        if (!processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
        if (!container_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyTerminatedError());
        const system::lxc::Lxc::State state = container_->state();
        if (state != system::lxc::Lxc::State::FROZEN)
            BOOST_THROW_EXCEPTION(
                ContainerIllegalStateError() <<
                system::lxc::IllegalStateError::state(state));
        container_->unfreeze();
    }

    boost::optional<ProcessGroup::Result> ProcessGroup::poll()
    {
        if (!processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
        if (!result_)
            result_ = processGroup_.poll();
        if (result_)
        {
            container_.reset();
            return result_->processGroupResult;
        }
        else
        {
            return boost::optional<ProcessGroup::Result>();
        }
    }

    const ProcessGroup::Result &ProcessGroup::wait()
    {
        if (!processGroup_)
            BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
        if (!result_)
        {
            const system::lxc::Lxc::State state = container_->state();
            if (state == system::lxc::Lxc::State::FROZEN)
                BOOST_THROW_EXCEPTION(
                    ContainerIllegalStateError() <<
                    system::lxc::IllegalStateError::state(state));
            result_ = processGroup_.wait();
            container_.reset();
        }
        BOOST_ASSERT(result_);
        return result_->processGroupResult;
    }

    const ProcessGroup::Result &ProcessGroup::result()
    {
        if (!result_)
        {
            if (container_)
                BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
            else
                BOOST_THROW_EXCEPTION(ProcessGroupHasNotTerminatedError());
        }
        return result_->processGroupResult;
    }

    const ProcessGroup::ResourceLimits &ProcessGroup::resourceLimits() const
    {
        return task_.resourceLimits;
    }

    void ProcessGroup::setResourceLimits(
        const ProcessGroup::ResourceLimits &resourceLimits)
    {
        task_.resourceLimits = resourceLimits;
    }

    const process::DefaultSettings &ProcessGroup::processDefaultSettings() const
    {
        return processDefaultSettings_;
    }

    void ProcessGroup::setProcessDefaultSettings(
        const process::DefaultSettings &processDefaultSettings)
    {
        processDefaultSettings_ = processDefaultSettings;
    }

    ProcessTask &ProcessGroup::processTask(const std::size_t id)
    {
        BOOST_ASSERT(id < task_.processes.size());
        return task_.processes[id];
    }

    const process::Result &ProcessGroup::processResult(const std::size_t id)
    {
        if (!result_)
        {
            if (container_)
                BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
            else
                BOOST_THROW_EXCEPTION(ProcessGroupHasNotTerminatedError());
        }
        BOOST_ASSERT(result_->processResults.size() == task_.processes.size());
        BOOST_ASSERT(id < result_->processResults.size());
        return result_->processResults[id];
    }
}}}
