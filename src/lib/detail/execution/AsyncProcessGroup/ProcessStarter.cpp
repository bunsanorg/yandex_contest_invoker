#include "ProcessStarter.hpp"

#include "Streams.hpp"

#include <yandex/contest/system/cgroup/CpuSet.hpp>
#include <yandex/contest/system/cgroup/Memory.hpp>
#include <yandex/contest/system/cgroup/MemorySwap.hpp>
#include <yandex/contest/system/unistd/access/Operations.hpp>
#include <yandex/contest/system/unistd/Operations.hpp>

#include <yandex/contest/detail/LogHelper.hpp>
#include <yandex/contest/detail/NullLog.hpp>
#include <yandex/contest/SystemError.hpp>

#include <boost/assert.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <signal.h>
#include <sys/resource.h>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    const int SIG_START_FAILED = SIGUSR2;

    struct FdAliasError: virtual Error
    {
        typedef boost::error_info<struct fdTag, int> fd;
    };

    struct UnresolvedFdAliasError: virtual FdAliasError {};

    struct InvalidTargetFdAliasError: virtual FdAliasError {};

    ProcessStarter::ProcessStarter(system::cgroup::ControlGroup &controlGroup,
                                   const AsyncProcessGroup::Process &process,
                                   std::vector<system::unistd::Pipe> &pipes):
        controlGroup_(controlGroup),
        ownerId_(process.ownerId),
        exec_(process.executable, process.arguments, process.environment),
        currentPath_(process.currentPath),
        resourceLimits_(process.resourceLimits)
    {
        setUpControlGroup();
        // TODO check that 0, 1, 2 are allocated
        std::unordered_set<int> childUsesFds;
        const Streams streams(
            pipes,
            allocatedFds_,
            process.currentPath,
            descriptors_
        );
        auto addStream =
            [this, &process, &streams, &childUsesFds](
                const std::pair<int, Stream> &fdStream,
                const bool isAlias)
            {
                if (isAlias)
                {
                    const AsyncProcessGroup::FdAlias *const fdAlias =
                        boost::get<const AsyncProcessGroup::FdAlias>(&fdStream.second);
                    BOOST_ASSERT(fdAlias);
                    auto iter = process.descriptors.find(fdAlias->fd);
                    if (iter == process.descriptors.end())
                        BOOST_THROW_EXCEPTION(UnresolvedFdAliasError() <<
                                              FdAliasError::fd(fdAlias->fd));
                    if (streams.isAlias(iter->second))
                        BOOST_THROW_EXCEPTION(InvalidTargetFdAliasError() <<
                                              FdAliasError::fd(fdAlias->fd));
                }
                const int fd = streams.getFd(fdStream.second);
                descriptors_[fdStream.first] = fd;
                BOOST_ASSERT(childUsesFds.find(fd) == childUsesFds.end());
                childUsesFds.insert(fd);
            };
        for (const auto &fdStream: process.descriptors)
        {
            if (!streams.isAlias(fdStream.second))
                addStream(fdStream, false);
        }
        for (const auto &fdStream: process.descriptors)
        {
            if (streams.isAlias(fdStream.second))
                addStream(fdStream, true);
        }
        // we do not want child process
        // to interfere with parent
        // so inherited fds should be closed
        for (int fd = 0; fd <= 2; ++fd)
            childCloseFds_.insert(fd);
        // close all not used fds
        // note: all files are used, so we will check only pipes
        for (const system::unistd::Pipe &pipe: pipes)
        {
            BOOST_ASSERT(pipe.readEndIsOpened());
            if (childUsesFds.find(pipe.readEnd()) == childUsesFds.end())
                childCloseFds_.insert(pipe.readEnd());
            BOOST_ASSERT(pipe.writeEndIsOpened());
            if (childUsesFds.find(pipe.writeEnd()) == childUsesFds.end())
                childCloseFds_.insert(pipe.writeEnd());
        }
    }

    Pid ProcessStarter::operator()()
    {
        const Pid pid = system::unistd::fork();
        BOOST_ASSERT(pid >= 0);
        if (pid > 0)
        { // parent
            STREAM_TRACE << "Child process was started pid = " << pid << ".";
            return pid;
        }
        else
        { // child
            startChild();
        }
        BOOST_ASSERT_MSG(false, "Never get here.");
        return -1;
    }

    /// Never returns.
    void ProcessStarter::startChild() noexcept
    {
        try
        {
            {
                // FIXME workaround, should be transmitted to control process
                const LogPointer log(new contest::detail::NullLog);
                Log::registerInstance(log);
            }
            childSetUpFds();
            // TODO verify has permissions to execute
            childSetUpResourceLimits();
            // note: we change current path before privileges drop
            // because we want to permit set arbitrary current path
            boost::filesystem::current_path(currentPath_);
            // We should not take into account our cpu time.
            // But it is not possible to attach task without a hack
            // not being root. So, attach it just before dropId() call.
            controlGroup_.attachTask(system::unistd::getpid());
            system::unistd::access::dropId(ownerId_);
            childSetUpResourceLimitsUser();
            // TODO usePath?
            exec_.execvpe();
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            ::raise(SIG_START_FAILED);
        }
        catch (...)
        {
            // FIXME should be indicated to parent
            BOOST_ASSERT_MSG(false, "It should not happen, but should be checked.");
        }
    }

    void ProcessStarter::childCloseFds()
    {
        for (const int fd: childCloseFds_)
            system::unistd::close(fd);
    }

    class DescriptorMonitor: private boost::noncopyable
    {
    public:
        DescriptorMonitor():
            maxFd_(system::unistd::getdtablesize()),
            requiredFds_(maxFd_),
            usedFds_(maxFd_),
            freeFds_(maxFd_),
            fdMap_(maxFd_)
        {
            freeFds_.flip(); // freeFds = "111...11";
        }

        void setRequired(const int fd)
        {
            requiredFds_.set(fd);
            freeFds_.reset(fd);
        }

        void setUsed(const int fd)
        {
            usedFds_.set(fd);
            freeFds_.reset(fd);
        }

        void closeFd(const int fd)
        {
            BOOST_ASSERT(usedFds_.test(fd));
            system::unistd::close(fd);
            usedFds_.reset(fd);
            freeFds_.set(!requiredFds_.test(fd));
        }

        void copyFd(const int oldfd, const int newfd)
        {
            BOOST_ASSERT(freeFds_.test(newfd) || requiredFds_.test(newfd));
            system::unistd::dup2(oldfd, newfd);
            freeFds_.reset(newfd);
            usedFds_.set(newfd);
        }

        void moveFd(const int oldfd, const int newfd)
        {
            copyFd(oldfd, newfd);
            closeFd(oldfd);
        }

        void freeRequiredFds()
        {
            boost::dynamic_bitset<> mustBeFreedFds = usedFds_ & requiredFds_;
            for (std::size_t i = mustBeFreedFds.find_first(),
                             freeFd = freeFds_.find_first();
                i != boost::dynamic_bitset<>::npos;
                i = mustBeFreedFds.find_next(i), freeFd = freeFds_.find_next(freeFd))
            {
                BOOST_ASSERT(freeFd != boost::dynamic_bitset<>::npos);
                moveFd(i, freeFd);
                mustBeFreedFds.reset(i);
                fdMap_[i] = freeFd;
                BOOST_ASSERT(!requiredFds_.test(freeFd));
            }
            BOOST_ASSERT(mustBeFreedFds.none());
            BOOST_ASSERT(!usedFds_.intersects(requiredFds_));
        }

        int getMappedFd(const int fd)
        {
            BOOST_ASSERT(0 < fd && static_cast<unsigned>(fd) < fdMap_.size());
            return fdMap_[fd] ? fdMap_[fd] : fd;
        }

        void moveMappedFd(const int oldfd, const int newfd)
        {
            moveFd(getMappedFd(oldfd), newfd);
        }

        ~DescriptorMonitor()
        {
            BOOST_ASSERT(usedFds_ == requiredFds_);
        }

    private:
        const unsigned maxFd_;
        boost::dynamic_bitset<> requiredFds_, usedFds_, freeFds_;
        std::vector<int> fdMap_;
    };

    void ProcessStarter::childSetUpFds()
    {
        // close unused descriptors
        childCloseFds();
        /*
         * Here we need to move allocated resources
         * to descriptors that are not required
         * to be set.
         */
        DescriptorMonitor fdMonitor;
        for (const auto &fdStream: descriptors_)
        {
            fdMonitor.setRequired(fdStream.first);
            fdMonitor.setUsed(fdStream.second);
        }
        fdMonitor.freeRequiredFds();
        for (const auto &fdStream: descriptors_)
            fdMonitor.moveMappedFd(fdStream.second, fdStream.first);
    }

    void ProcessStarter::setUpControlGroup()
    {
        system::cgroup::ControlGroup parentCG = controlGroup_.parent();

        const system::cgroup::CpuSet parentCpuSet(parentCG), cpuSet(controlGroup_);
        cpuSet.setCpus(parentCpuSet.cpus());
        cpuSet.setMems(parentCpuSet.mems());

        const system::cgroup::Memory memory(controlGroup_);

        // we do not want to count memory used by control process
        cpuSet.setMemoryMigrate(false);
        memory.setMoveChargeAtImmigrate(false, false);

        // we need oom-killer
        memory.setOomKillDisable(false);
    }

    void ProcessStarter::childSetUpResourceLimits()
    {
        ::rlimit rlim;

        // output limit (in bytes)
        rlim.rlim_cur = rlim.rlim_max = boost::numeric_cast<rlim_t>(
            resourceLimits_.outputLimitBytes);
        system::unistd::setrlimit(RLIMIT_FSIZE, rlim);

        // stack
        rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
        system::unistd::setrlimit(RLIMIT_STACK, rlim);
    }

    void ProcessStarter::childSetUpResourceLimitsUser()
    {
        ::rlimit rlim;

        // number of processes
        rlim.rlim_cur = rlim.rlim_max = boost::numeric_cast<rlim_t>(
            resourceLimits_.numberOfProcesses);
        system::unistd::setrlimit(RLIMIT_NPROC, rlim);
    }
}}}}}}
