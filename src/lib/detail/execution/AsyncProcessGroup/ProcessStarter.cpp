#include "ProcessStarter.hpp"
#include "Streams.hpp"

#include "yandex/contest/SystemError.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"
#include "yandex/contest/system/unistd/access/Operations.hpp"

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

    struct InvalidTargetFDAliasError: FDAliasError {};

    ProcessStarter::ProcessStarter(const AsyncProcessGroup::Process &process,
                                   std::vector<system::unistd::Pipe> &pipes):
        ownerId_(process.ownerId),
        exec_(process.executable, process.arguments, process.environment),
        currentPath_(process.currentPath),
        resourceLimits_(process.resourceLimits)
    {
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
            system::unistd::access::dropId(ownerId_);
            childSetUpResourceLimitsUser();
            // TODO usePath?
            exec_.execvpe();
            BOOST_THROW_EXCEPTION(SystemError("execvpe"));
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
            freeFDs_.reset(0); // 0 is not valid descriptor
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

    void ProcessStarter::childSetUpResourceLimits()
    {
        ::rlimit rlim;

        // cpu limit (in seconds)
        // FIXME "1 +" fix should be replaced in the future
        // by something with higher precision
        rlim.rlim_cur = rlim.rlim_max = 1 + boost::numeric_cast<rlim_t>(
            (resourceLimits_.timeLimitMillis + 999) / 1000);
        system::unistd::setrlimit(RLIMIT_CPU, rlim);

        // hard memory limit (in bytes)
        rlim.rlim_cur = rlim.rlim_max = boost::numeric_cast<rlim_t>(
            resourceLimits_.hardMemoryLimitBytes);
        system::unistd::setrlimit(RLIMIT_AS, rlim);

        // output limit (in bytes)
        rlim.rlim_cur = rlim.rlim_max = boost::numeric_cast<rlim_t>(
            resourceLimits_.outputLimitBytes);
        system::unistd::setrlimit(RLIMIT_FSIZE, rlim);
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
