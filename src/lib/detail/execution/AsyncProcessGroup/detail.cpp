#include <yandex/contest/invoker/detail/execution/AsyncProcessGroupDetail.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    File::File(const boost::filesystem::path &path_,
               const AccessMode accessMode_):
        path(path_),
        accessMode(accessMode_) {}

    Pipe::Pipe(const std::size_t id): pipeId(id) {}

    Pipe::End Pipe::end(const Pipe::End::Type type) const
    {
        return End{pipeId, type};
    }

    Pipe::End Pipe::readEnd() const
    {
        return end(End::READ);
    }

    Pipe::End Pipe::writeEnd() const
    {
        return end(End::WRITE);
    }

    FDAlias::FDAlias(const int fd_): fd(fd_) {}
}}}}}}
