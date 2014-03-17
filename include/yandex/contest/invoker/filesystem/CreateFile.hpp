#pragma once

#include <yandex/contest/invoker/filesystem/Device.hpp>
#include <yandex/contest/invoker/filesystem/Directory.hpp>
#include <yandex/contest/invoker/filesystem/Fifo.hpp>
#include <yandex/contest/invoker/filesystem/File.hpp>
#include <yandex/contest/invoker/filesystem/RegularFile.hpp>
#include <yandex/contest/invoker/filesystem/SymLink.hpp>

#include <bunsan/config/traits.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/variant.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    class CreateFile
    {
    public:
        CreateFile()=default;

        template <typename T>
        explicit CreateFile(const T &file): file_(file) {}

        template <typename T>
        CreateFile &operator=(const T &file)
        {
            file_ = file;
            return *this;
        }

        CreateFile(const CreateFile &)=default;
        CreateFile &operator=(const CreateFile &)=default;

        /// \copydoc File::create()
        void create() const;

        /// Call create() relative to root.
        void create(const boost::filesystem::path &root) const;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            // Do not use nvp here
            // (this class is wrapper,
            // we want to serialize field).
            ar & file_;
        }

    public:
        typedef boost::variant<
            RegularFile,
            SymLink,
            Device,
            Directory,
            Fifo
        > Variant;

    private:
        Variant file_;
    };

    typedef std::vector<CreateFile> CreateFiles;
}}}}

BUNSAN_CONFIG_EXPORT(
    yandex::contest::invoker::filesystem::CreateFile::Variant,
    yandex::contest::invoker::filesystem::RegularFile, "RegularFile")

BUNSAN_CONFIG_EXPORT(
    yandex::contest::invoker::filesystem::CreateFile::Variant,
    yandex::contest::invoker::filesystem::SymLink, "SymLink")

BUNSAN_CONFIG_EXPORT(
    yandex::contest::invoker::filesystem::CreateFile::Variant,
    yandex::contest::invoker::filesystem::Device, "Device")

BUNSAN_CONFIG_EXPORT(
    yandex::contest::invoker::filesystem::CreateFile::Variant,
    yandex::contest::invoker::filesystem::Directory, "Directory")

BUNSAN_CONFIG_EXPORT(
    yandex::contest::invoker::filesystem::CreateFile::Variant,
    yandex::contest::invoker::filesystem::Fifo, "Fifo")
