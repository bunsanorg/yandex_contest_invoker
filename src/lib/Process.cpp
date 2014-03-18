#include <yandex/contest/invoker/Process.hpp>

#include <yandex/contest/invoker/ContainerError.hpp>
#include <yandex/contest/invoker/ProcessGroup.hpp>

#include <yandex/contest/detail/IntrusivePointerHelper.hpp>

#include <boost/assert.hpp>
#include <boost/variant/static_visitor.hpp>

namespace yandex{namespace contest{namespace invoker
{
    YANDEX_CONTEST_INTRUSIVE_PTR_DEFINE(Process)

    ProcessPointer Process::create(
        const ProcessGroupPointer &processGroup,
        const std::size_t id)
    {
        ProcessPointer ret(new Process(processGroup, id));
        processGroup->processDefaultSettings().setUpProcess(ret);
        return ret;
    }

    Process::Process(const ProcessGroupPointer &processGroup, const std::size_t id):
        processGroup_(processGroup),
        id_(id)
    {
        BOOST_ASSERT(processGroup_);
    }

    const boost::filesystem::path &Process::executable() const
    {
        return processGroup_->processTask(id_).executable;
    }

    bool Process::groupWaitsForTermination() const
    {
        return processGroup_->processTask(id_).groupWaitsForTermination;
    }

    void Process::setGroupWaitsForTermination(
        const bool groupWaitsForTermination)
    {
        processGroup_->processTask(id_).groupWaitsForTermination =
            groupWaitsForTermination;
    }

    bool Process::terminateGroupOnCrash() const
    {
        return processGroup_->processTask(id_).terminateGroupOnCrash;
    }

    void Process::setTerminateGroupOnCrash(const bool terminateGroupOnCrash)
    {
        processGroup_->processTask(id_).terminateGroupOnCrash =
            terminateGroupOnCrash;
    }

    const ProcessArguments &Process::arguments() const
    {
        return processGroup_->processTask(id_).arguments;
    }

    void Process::setArguments(const ProcessArguments &arguments)
    {
        processGroup_->processTask(id_).arguments = arguments;
    }

    const boost::filesystem::path &Process::currentPath() const
    {
        return processGroup_->processTask(id_).currentPath;
    }

    void Process::setCurrentPath(const boost::filesystem::path &currentPath)
    {
        processGroup_->processTask(id_).currentPath = currentPath;
    }

    const ProcessEnvironment &Process::environment() const
    {
        return processGroup_->processTask(id_).environment;
    }

    void Process::setEnvironment(const ProcessEnvironment &environment)
    {
        processGroup_->processTask(id_).environment = environment;
    }

    const Process::ResourceLimits &Process::resourceLimits() const
    {
        return processGroup_->processTask(id_).resourceLimits;
    }

    void Process::setResourceLimits(
        const Process::ResourceLimits &resourceLimits)
    {
        processGroup_->processTask(id_).resourceLimits = resourceLimits;
    }

    const system::unistd::access::Id &Process::ownerId() const
    {
        return processGroup_->processTask(id_).ownerId;
    }

    void Process::setOwnerId(const system::unistd::access::Id &ownerId)
    {
        processGroup_->processTask(id_).ownerId = ownerId;
    }

    const std::string &Process::name() const
    {
        return processGroup_->processTask(id_).name;
    }

    void Process::setName(const std::string &name)
    {
        processGroup_->processTask(id_).name = name;
    }

    void Process::setStream(const int descriptor, const Stream &stream)
    {
        processGroup_->processTask(id_).descriptors[descriptor] = stream;
    }

    namespace
    {
        struct NonPipeStreamToStreamVisitor: boost::static_visitor<Stream>
        {
            template <typename T>
            Stream operator()(const T &obj) const
            {
                return obj;
            }
        };
    }

    void Process::setNonPipeStream(
        const int descriptor,
        const NonPipeStream &stream)
    {
        setStream(
            descriptor,
            boost::apply_visitor(NonPipeStreamToStreamVisitor(), stream)
        );
    }

    Stream Process::stream(const int descriptor) const
    {
        const auto iter =
            processGroup_->processTask(id_).descriptors.find(descriptor);

        if (iter == processGroup_->processTask(id_).descriptors.end())
        {
            BOOST_THROW_EXCEPTION(
                ProcessDescriptorOutOfRangeError() <<
                ProcessDescriptorOutOfRangeError::descriptor(descriptor));
        }
        else
        {
            return iter->second;
        }
    }

    void Process::closeStream(const int descriptor)
    {
        processGroup_->processTask(id_).descriptors.erase(descriptor);
    }

    bool Process::hasStream(const int descriptor) const
    {
        const auto iter =
            processGroup_->processTask(id_).descriptors.find(descriptor);
        return iter != processGroup_->processTask(id_).descriptors.end();
    }

    const Process::Result &Process::result() const
    {
        return processGroup_->processResult(id_);
    }

    Process::Id Process::id() const
    {
        return id_;
    }
}}}
