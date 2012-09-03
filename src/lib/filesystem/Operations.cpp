#include "yandex/contest/invoker/filesystem/Operations.hpp"

#include <string>
#include <deque>

#include <boost/filesystem/operations.hpp>

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
}}}}
