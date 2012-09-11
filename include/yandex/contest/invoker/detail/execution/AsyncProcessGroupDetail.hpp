#pragma once

#include "yandex/contest/invoker/detail/CommonTypedefs.hpp"

#include "yandex/contest/StreamEnum.hpp"

#include "yandex/contest/config/TypeTraits.hpp"

#include "yandex/contest/invoker/process_group/ResourceLimits.hpp"
#include "yandex/contest/invoker/process_group/Result.hpp"

#include "yandex/contest/invoker/process/ResourceLimits.hpp"
#include "yandex/contest/invoker/process/Result.hpp"

#include "yandex/contest/system/unistd/access/Id.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>

#include <boost/variant.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include "yandex/contest/serialization/path.hpp"
#include "yandex/contest/serialization/unordered_map.hpp"

namespace yandex{namespace contest{namespace invoker{namespace detail{namespace execution{namespace async_process_group_detail
{
    YANDEX_CONTEST_STREAM_ENUM_CLASS(AccessMode,
    (
        READ_ONLY,
        WRITE_ONLY,
        READ_WRITE
    ))

    struct File
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(path);
            ar & BOOST_SERIALIZATION_NVP(accessMode);
        }

        explicit File(const boost::filesystem::path &path_=boost::filesystem::path(),
                      const AccessMode accessMode_=AccessMode::READ_WRITE);
        File(const File &)=default;
        File &operator=(const File &)=default;

        boost::filesystem::path path;
        AccessMode accessMode;
    };

    struct Pipe
    {
        struct End
        {
            template <typename Archive>
            void serialize(Archive &ar, const unsigned int)
            {
                ar & BOOST_SERIALIZATION_NVP(pipeId);
                ar & BOOST_SERIALIZATION_NVP(end);
            }

            YANDEX_CONTEST_INCLASS_STREAM_ENUM(Type,
            (
                READ,
                WRITE
            ))

            std::size_t pipeId;
            Type end;
        };

        explicit Pipe(const std::size_t id);
        Pipe(const Pipe &)=default;
        Pipe &operator=(const Pipe &)=default;

        std::size_t pipeId;

        End end(const End::Type type) const;
        End readEnd() const;
        End writeEnd() const;
    };

    struct FDAlias
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(fd);
        }

        explicit FDAlias(const int fd_=0);

        FDAlias(const FDAlias &)=default;
        FDAlias &operator=(const FDAlias &)=default;

        int fd;
    };

    typedef boost::variant<Pipe::End, File, FDAlias> Stream;
    typedef boost::variant<File, FDAlias> NonPipeStream;

    struct Process
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(descriptors);
            ar & BOOST_SERIALIZATION_NVP(resourceLimits);
            ar & BOOST_SERIALIZATION_NVP(executable);
            ar & BOOST_SERIALIZATION_NVP(arguments);
            ar & BOOST_SERIALIZATION_NVP(currentPath);
            ar & BOOST_SERIALIZATION_NVP(environment);
            ar & BOOST_SERIALIZATION_NVP(ownerId);
            ar & BOOST_SERIALIZATION_NVP(groupWaitsForTermination);
            ar & BOOST_SERIALIZATION_NVP(terminateGroupOnCrash);
        }

        typedef std::unordered_map<int, Stream> DescriptorMap;

        DescriptorMap descriptors;
        process::ResourceLimits resourceLimits;
        boost::filesystem::path executable;
        ProcessArguments arguments;
        boost::filesystem::path currentPath = "/";
        ProcessEnvironment environment;
        system::unistd::access::Id ownerId;
        bool groupWaitsForTermination = true;
        bool terminateGroupOnCrash = true;
    };

    struct Task
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(processes);
            ar & BOOST_SERIALIZATION_NVP(pipesNumber);
            ar & BOOST_SERIALIZATION_NVP(resourceLimits);
        }

        std::vector<Process> processes;

        // we have nothing to store here
        // we need to know only number of pipes to be allocated
        std::size_t pipesNumber = 0;

        process_group::ResourceLimits resourceLimits;
    };

    std::istream &operator>>(std::istream &in, Task &task);
    std::ostream &operator<<(std::ostream &out, const Task &task);

    struct Result
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(processResults);
            ar & BOOST_SERIALIZATION_NVP(processGroupResult);
        }

        std::vector<process::Result> processResults;
        process_group::Result processGroupResult;
    };
}}}}}}

YANDEX_CONTEST_CONFIG_EXPORT(
    yandex::contest::invoker::detail::execution::async_process_group_detail::Stream,
    yandex::contest::invoker::detail::execution::async_process_group_detail::File, "File")

YANDEX_CONTEST_CONFIG_EXPORT(
    yandex::contest::invoker::detail::execution::async_process_group_detail::Stream,
    yandex::contest::invoker::detail::execution::async_process_group_detail::Pipe::End, "PipeEnd")

YANDEX_CONTEST_CONFIG_EXPORT(
    yandex::contest::invoker::detail::execution::async_process_group_detail::Stream,
    yandex::contest::invoker::detail::execution::async_process_group_detail::FDAlias, "FDAlias")

YANDEX_CONTEST_CONFIG_EXPORT(
    yandex::contest::invoker::detail::execution::async_process_group_detail::NonPipeStream,
    yandex::contest::invoker::detail::execution::async_process_group_detail::File, "File")

YANDEX_CONTEST_CONFIG_EXPORT(
    yandex::contest::invoker::detail::execution::async_process_group_detail::NonPipeStream,
    yandex::contest::invoker::detail::execution::async_process_group_detail::FDAlias, "FDAlias")
