// warning: need to be the first include
#include <bunsan/config.hpp>

#include <yandex/contest/invoker/filesystem/Error.hpp>
#include <yandex/contest/invoker/filesystem/RegularFile.hpp>

#include <yandex/contest/system/unistd/Operations.hpp>

#include <bunsan/enable_error_info.hpp>
#include <bunsan/filesystem/fstream.hpp>

#include <boost/filesystem/operations.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    void RegularFile::mknod() const
    {
        if (source)
        {
            if (!boost::filesystem::exists(source.get()))
                BOOST_THROW_EXCEPTION(SourceDoesNotExistsError() <<
                                      InvalidSourceError::source(source.get()));
            if (!boost::filesystem::is_regular_file(source.get()))
                BOOST_THROW_EXCEPTION(SourceIsNotRegularFileError() <<
                                      InvalidSourceError::source(source.get()));
            boost::filesystem::copy_file(
                source.get(), path,
                boost::filesystem::copy_option::overwrite_if_exists);
        }
        else
        {
            BUNSAN_EXCEPTIONS_WRAP_BEGIN()
            {
                bunsan::filesystem::ofstream fout(path);
                fout.close();
            }
            BUNSAN_EXCEPTIONS_WRAP_END()
        }
        chmod();
    }
}}}}
