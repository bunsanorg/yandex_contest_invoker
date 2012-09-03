// hack to get correct signature for boost::filesystem::copy_file()
#include <boost/config.hpp>
#define BOOST_NO_SCOPED_ENUMS

#include "yandex/contest/invoker/filesystem/RegularFile.hpp"
#include "yandex/contest/invoker/filesystem/Error.hpp"

#include "yandex/contest/SystemError.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

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
            boost::filesystem::ofstream fout(path);
            if (!fout)
                BOOST_THROW_EXCEPTION(SystemError("open"));
            fout.close();
            if (!fout)
                BOOST_THROW_EXCEPTION(SystemError("close"));
        }
        chmod();
    }
}}}}
