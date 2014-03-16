#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>

#include <yandex/contest/detail/LogHelper.hpp>
#include <yandex/contest/SerializationCast.hpp>

#include <boost/assert.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution
{
    namespace
    {
        AsyncProcess::Options apply(
            AsyncProcess::Options options,
            const AsyncProcessGroup::Task &task)
        {
            STREAM_TRACE << "Attempt to execute process group: " <<
                            STREAM_OBJECT(task);
            options.in = serialization::serialize(task);
            return options;
        }
    }

    AsyncProcessGroup::AsyncProcessGroup(
        const AsyncProcess::Options &options, const Task &task):
            controlProcess_(apply(options, task))
    {
    }

    AsyncProcessGroup::AsyncProcessGroup(AsyncProcessGroup &&processGroup)
    {
        swap(processGroup);
    }

    AsyncProcessGroup &AsyncProcessGroup::operator=(
        AsyncProcessGroup &&processGroup)
    {
        if (*this)
            stop();
        swap(processGroup);
        return *this;
    }

    AsyncProcessGroup::operator bool() const noexcept
    {
        return static_cast<bool>(controlProcess_);
    }

    void AsyncProcessGroup::swap(AsyncProcessGroup &processGroup) noexcept
    {
        using boost::swap;
        swap(controlProcess_, processGroup.controlProcess_);
        swap(result_, processGroup.result_);
    }

    const AsyncProcessGroup::Result &AsyncProcessGroup::wait()
    {
        BOOST_ASSERT_MSG(*this, "Invalid AsyncProcessGroup instance.");
        if (!result_)
            readResult();
        BOOST_ASSERT(result_);
        return result_.get();
    }

    const boost::optional<AsyncProcessGroup::Result> &AsyncProcessGroup::poll()
    {
        BOOST_ASSERT_MSG(*this, "Invalid AsyncProcessGroup instance.");
        if (!result_ && controlProcess_.poll())
            readResult();
        return result_;
    }

    void AsyncProcessGroup::stop()
    {
        BOOST_ASSERT_MSG(*this, "Invalid AsyncProcessGroup instance.");
        controlProcess_.stop();
        readResult();
    }

    void AsyncProcessGroup::readResult()
    {
        const execution::Result result = controlProcess_.wait();
        if (result)
        {
            try
            {
                result_ = serialization::deserialize<Result>(result.out);
            }
            catch (std::exception &)
            {
                BOOST_THROW_EXCEPTION(
                    AsyncProcessGroupControlProcessError(result) <<
                    bunsan::enable_nested_current());
            }
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                AsyncProcessGroupControlProcessError(result));
        }
    }

    AsyncProcessGroup::~AsyncProcessGroup()
    {
        if (*this)
        {
            try
            {
                stop();
            }
            catch (std::exception &e)
            {
            }
        }
    }
}}}}}
