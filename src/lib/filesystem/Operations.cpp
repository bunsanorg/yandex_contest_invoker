#include <yandex/contest/invoker/filesystem/Operations.hpp>

#include <yandex/contest/invoker/filesystem/Error.hpp>

#include <boost/filesystem/operations.hpp>

#include <deque>
#include <string>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    boost::filesystem::path keepInRoot(const boost::filesystem::path &path,
                                       const boost::filesystem::path &root)
    {
        std::deque<boost::filesystem::path> stack;
        for (const boost::filesystem::path &i: path)
        {
            if (i == "..")
            {
                if (!stack.empty())
                    stack.pop_back();
            }
            else if (i != ".")
            {
                stack.push_back(i);
            }
        }
        boost::filesystem::path stripped;
        for (const boost::filesystem::path &i: stack)
            stripped /= i;
        return root / stripped;
    }

    boost::filesystem::path containerPath(const boost::filesystem::path &path,
                                          const boost::filesystem::path &root)
    {
        if (root.is_relative())
            BOOST_THROW_EXCEPTION(
                InvalidContainerRootError() <<
                InvalidContainerRootError::path(root));

        boost::filesystem::path relative = "/";
        auto r = root.begin();
        auto p = path.begin();
        for (; r != root.end() && p != path.end() && *r == *p; ++r, ++p)
            continue;
        if (r == root.end())
        {
            for (; p != path.end(); ++p)
                relative /= *p;
            return relative;
        }

        BOOST_THROW_EXCEPTION(
            NonContainerPathError() <<
            NonContainerPathError::path(path));
    }
}}}}
