#pragma once

#include <yandex/contest/invoker/Forward.hpp>

#include <yandex/contest/invoker/process/ResourceLimits.hpp>

#include <yandex/contest/system/unistd/access/Id.hpp>

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace process
{
    struct DefaultSettings
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(resourceLimits);
            ar & BOOST_SERIALIZATION_NVP(environment);
            ar & BOOST_SERIALIZATION_NVP(currentPath);
            ar & BOOST_SERIALIZATION_NVP(ownerId);
            ar & BOOST_SERIALIZATION_NVP(descriptors);
        }

        ResourceLimits resourceLimits;
        ProcessEnvironment environment;
        boost::filesystem::path currentPath;
        system::unistd::access::Id ownerId;

        /// \note It is not possible to use Pipe::End here.
        std::unordered_map<int, NonPipeStream> descriptors;

        void setUpProcess(const ProcessPointer &process) const;
    };
}}}}
