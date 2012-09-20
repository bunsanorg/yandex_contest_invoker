#include "ProcessStarter.hpp"
#include "Streams.hpp"

#include "yandex/contest/SystemError.hpp"

#include "yandex/contest/detail/LogHelper.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"
#include "yandex/contest/system/unistd/access/Operations.hpp"

#include "yandex/contest/system/cgroup/CpuSet.hpp"
#include "yandex/contest/system/cgroup/Memory.hpp"
#include "yandex/contest/system/cgroup/MemorySwap.hpp"

#include <boost/assert.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/filesystem/operations.hpp>

#include <signal.h>

#include <sys/resource.h>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    const int SIG_START_FAILED = SIGUSR2;

    struct FDAliasError: virtual Error
    {
        typedef boost::error_info<struct fdTag, int> fd;
    };

    struct UnresolvedFDAliasError: virtual FDAliasError {};

    struct InvalidTargetFDAliasError: virtual FDAliasError {};

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
        std::unordered_set<int> childUsesFDs;
        const Streams streams(pipes, allocatedFDs_, process.currentPath, descriptors_);
        auto addStream =
            [this, &process, &streams, &childUsesFDs](const std::pair<int, Stream> &fdStream,
                                                      const bool isAlias)
            {
                if (isAlias)
                {
                    const AsyncProcessGroup::FDAlias *const fdAlias =
                        boost::get<const AsyncProcessGroup::FDAlias>(&fdStream.second);
                    BOOST_ASSERT(fdAlias);
                    auto iter = process.descriptors.find(fdAlias->fd);
                    if (iter == process.descriptors.end())
                        BOOST_THROW_EXCEPTION(UnresolvedFDAliasError() <<
                                              FDAliasError::fd(fdAlias->fd));
                    if (streams.isAlias(iter->second))
                        BOOST_THROW_EXCEPTION(InvalidTargetFDAliasError() <<
                                              FDAliasError::fd(fdAlias->fd));
                }
                const int fd = streams.getFD(fdStream.second);
                descriptors_[fdStream.first] = fd;
                BOOST_ASSERT(childUsesFDs.find(fd) == childUsesFDs.end());
                childUsesFDs.insert(fd);
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
            childCloseFDs_.insert(fd);
        // close all not used fds
        // note: all files are used, so we will check only pipes
        for (const system::unistd::Pipe &pipe: pipes)
        {
            BOOST_ASSERT(pipe.readEndIsOpened());
            if (childUsesFDs.find(pipe.readEnd()) == childUsesFDs.end())
                childCloseFDs_.insert(pipe.readEnd());
            BOOST_ASSERT(pipe.writeEndIsOpened());
            if (childUsesFDs.find(pipe.writeEnd()) == childUsesFDs.end())
                childCloseFDs_.insert(pipe.writeEnd());
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
            childSetUpFDs();
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

    void ProcessStarter::childCloseFDs()
    {
        for (const int fd: childCloseFDs_)
            system::unistd::close(fd);
    }

    class DescriptorMonitor: private boost::noncopyable
    {
    public:
        DescriptorMonitor():
            maxFD_(system::unistd::getdtablesize()),
            requiredFDs_(maxFD_),
            usedFDs_(maxFD_),
            freeFDs_(maxFD_),
            fdMap_(maxFD_)
        {
            freeFDs_.flip(); // freeFDs = "111...11";
        }

        void setRequired(const int fd)
        {
            requiredFDs_.set(fd);
            freeFDs_.reset(fd);
        }

        void setUsed(const int fd)
        {
            usedFDs_.set(fd);
            freeFDs_.reset(fd);
        }

        void closeFD(const int fd)
        {
            BOOST_ASSERT(usedFDs_.test(fd));
            system::unistd::close(fd);
            usedFDs_.reset(fd);
            freeFDs_.set(!requiredFDs_.test(fd));
        }

        void copyFD(const int oldfd, const int newfd)
        {
            BOOST_ASSERT(freeFDs_.test(newfd) || requiredFDs_.test(newfd));
            system::unistd::dup2(oldfd, newfd);
            freeFDs_.reset(newfd);
            usedFDs_.set(newfd);
        }

        void moveFD(const int oldfd, const int newfd)
        {
            copyFD(oldfd, newfd);
            closeFD(oldfd);
        }

        void freeRequiredFDs()
        {
            boost::dynamic_bitset<> mustBeFreedFDs = usedFDs_ & requiredFDs_;
            for (std::size_t i = mustBeFreedFDs.find_first(), freeFD = freeFDs_.find_first();
                i != boost::dynamic_bitset<>::npos;
                i = mustBeFreedFDs.find_next(i), freeFD = freeFDs_.find_next(freeFD))
            {
                BOOST_ASSERT(freeFD != boost::dynamic_bitset<>::npos);
                moveFD(i, freeFD);
                mustBeFreedFDs.reset(i);
                fdMap_[i] = freeFD;
                BOOST_ASSERT(!requiredFDs_.test(freeFD));
            }
            BOOST_ASSERT(mustBeFreedFDs.none());
            BOOST_ASSERT(!usedFDs_.intersects(requiredFDs_));
        }

        int getMappedFD(const int fd)
        {
            BOOST_ASSERT(0 < fd && static_cast<unsigned>(fd) < fdMap_.size());
            return fdMap_[fd] ? fdMap_[fd] : fd;
        }

        void moveMappedFD(const int oldfd, const int newfd)
        {
            moveFD(getMappedFD(oldfd), newfd);
        }

        ~DescriptorMonitor()
        {
            BOOST_ASSERT(usedFDs_ == requiredFDs_);
        }

    private:
        const unsigned maxFD_;
        boost::dynamic_bitset<> requiredFDs_, usedFDs_, freeFDs_;
        std::vector<int> fdMap_;
    };

    void ProcessStarter::childSetUpFDs()
    {
        // close unused descriptors
        childCloseFDs();
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
        fdMonitor.freeRequiredFDs();
        for (const auto &fdStream: descriptors_)
            fdMonitor.moveMappedFD(fdStream.second, fdStream.first);
    }

    void ProcessStarter::setUpControlGroup()
    {
        system::cgroup::ControlGroup parentCG = controlGroup_.parent();

        const system::cgroup::CpuSet parentCpuSet(parentCG), cpuSet(controlGroup_);
        cpuSet.setCpus(parentCpuSet.cpus());
        cpuSet.setMems(parentCpuSet.mems());

        const system::cgroup::Memory memory(controlGroup_);
        const system::cgroup::MemorySwap memorySwap(controlGroup_);
        memory.setLimit(resourceLimits_.memoryLimitBytes);
        memorySwap.setLimit(resourceLimits_.memoryLimitBytes);

        // we do not want to count memory used by control process
        cpuSet.setMemoryMigrate(false);
        memory.setMoveChargeAtImmigrate(false, false);

        // wee need oom-killer
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
