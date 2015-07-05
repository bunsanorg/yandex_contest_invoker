#pragma once

#include <boost/noncopyable.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

#include <iostream>
#include <string>
#include <type_traits>

namespace yandex{namespace contest{namespace invoker{
    namespace lxc{namespace config
{
    class OutputArchive
    {
    public:
        typedef std::integral_constant<bool, false> is_loading;
        typedef std::integral_constant<bool, true> is_saving;

        unsigned int get_library_version() { return 0; }

    public:
        OutputArchive(std::ostream &out, const std::string &prefix):
            out_(&out),
            prefix_(prefix)
        {}

        template <typename T>
        OutputArchive &operator<<(const T &obj)
        {
            save(obj);
            return *this;
        }

        template <typename T>
        OutputArchive &operator<<(const boost::serialization::nvp<T> &nvp)
        {
            saveToStream(nvp.value(), out_, prefix_ + "." + nvp.name());
            return *this;
        }

        template <typename T>
        OutputArchive &operator<<(
            const boost::serialization::nvp<boost::optional<T>> &nvp)
        {
            using boost::serialization::make_nvp;

            if (nvp.value())
                *this << make_nvp(nvp.name(), nvp.value().get());
            return *this;
        }

        template <typename T>
        static void saveToStream(
            const T &obj, std::ostream &out, const std::string &prefix)
        {
            OutputArchive oa(out, prefix);
            oa << obj;
        }

    private:
        template <typename T>
        void save(const T &obj)
        {
        }

    private:
        std::ostream &out_;
        std::string prefix_;
    };
}}}}}
