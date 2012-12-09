#include "yandex/contest/invoker/ContainerConfig.hpp"
#include "yandex/contest/invoker/ConfigurationError.hpp"

#include "bunsan/config/input_archive.hpp"
#include "bunsan/config/output_archive.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace bunsan{namespace config{namespace traits
{
    template <>
    struct direct_assign<yandex::contest::system::unistd::MountEntry>:
        std::integral_constant<bool, true> {};
}}}

namespace boost{namespace property_tree
{
    template <>
    struct translator_between<std::string, yandex::contest::system::unistd::MountEntry>
    {
        struct type
        {
            typedef std::string internal_type;
            typedef yandex::contest::system::unistd::MountEntry external_type;

            boost::optional<external_type> get_value(const internal_type &s)
            {
                return static_cast<external_type>(s);
            }

            boost::optional<internal_type> put_value(const external_type &entry)
            {
                return static_cast<internal_type>(entry);
            }
        };
    };
}}

namespace yandex{namespace contest{namespace invoker
{
    namespace
    {
        constexpr unsigned getWordSize()
        {
            return 8 * sizeof(void *);
        }

        constexpr system::lxc::Config::Arch getArch()
        {
            static_assert(getWordSize() == 32 || getWordSize() == 64, "Unknown word size.");
            return getWordSize() == 32 ? system::lxc::Config::Arch::x86 : system::lxc::Config::Arch::x86_64;
        }

        system::lxc::MountConfig getLXCMountConfig()
        {
            system::lxc::MountConfig st;
            st.entries = {
                system::unistd::MountEntry::bindRO("/etc", "/etc"),
                system::unistd::MountEntry::bindRO("/bin", "/bin"),
                system::unistd::MountEntry::bindRO("/sbin", "/sbin"),
                system::unistd::MountEntry::bindRO("/lib", "/lib"),
                system::unistd::MountEntry::bindRO("/usr", "/usr"),
                system::unistd::MountEntry::proc()
            };
            return st;
        }

        system::lxc::Config getLXCConfig()
        {
            system::lxc::Config st;
            st.arch = getArch();
            st.utsname = "container";
            // TODO network
            // TODO pts
            // TODO console
            // TODO tty
            st.mount = getLXCMountConfig();
            // TODO rootfs
            // TODO cgroup
            // TODO cap_drop
            return st;
        }

        ControlProcessConfig getControlProcessConfig()
        {
            ControlProcessConfig st;
            st.executable = "yandex_contest_invoker_ctl";
            return st;
        }

        process::ResourceLimits getProcessResourceLimits()
        {
            return process::ResourceLimits();
        }

        process::DefaultSettings getProcessDefaultSettings()
        {
            process::DefaultSettings st;
            st.resourceLimits = getProcessResourceLimits();
            st.environment = {
                {"PATH", "/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin"},
                {"LC_ALL", "C"},
                {"LANG", "C"},
                {"PWD", "/"}
            };
            // FIXME should be nobody
            st.ownerId = {0, 0};
            // stdin, stdout, stderr
            for (int fd = 0; fd <= 2; ++fd)
                st.descriptors[fd] = File("/dev/null", AccessMode::READ_WRITE);
            return st;
        }

        process_group::ResourceLimits getProcessGroupResourceLimits()
        {
            return process_group::ResourceLimits();
        }

        process_group::DefaultSettings getProcessGroupDefaultSettings()
        {
            process_group::DefaultSettings st;
            st.processDefaultSettings = getProcessDefaultSettings();
            st.resourceLimits = getProcessGroupResourceLimits();
            return st;
        }

        filesystem::CreateFile getDevice(const boost::filesystem::path &path,
                                         const mode_t mode,
                                         const filesystem::Device::Type type,
                                         const int major,
                                         const int minor)
        {
            filesystem::Device device;
            device.path = path;
            device.mode = mode;
            device.type = type;
            device.major = major;
            device.minor = minor;
            return filesystem::CreateFile(device);
        }

        filesystem::CreateFile getCharDevice(const boost::filesystem::path &path,
                                             const mode_t mode,
                                             const int major,
                                             const int minor)
        {
            return getDevice(path, mode, filesystem::Device::CHAR, major, minor);
        }

        filesystem::CreateFile getSymLink(const boost::filesystem::path &value,
                                          const boost::filesystem::path &path)
        {
            filesystem::SymLink symlink;
            symlink.value = value;
            symlink.path = path;
            return filesystem::CreateFile(symlink);
        }

        filesystem::Config getFilesystemConfig()
        {
            filesystem::Config st;
            st.createFiles = {
                getCharDevice("/dev/null", 0666, 1, 3),
                getCharDevice("/dev/zero", 0666, 1, 5),
                getCharDevice("/dev/random", 0666, 1, 8),
                getCharDevice("/dev/urandom", 0666, 1, 9),
                getCharDevice("/dev/full", 0666, 1, 7),
                getSymLink("/proc/fd", "/dev/fd"),
                getSymLink("/proc/self/fd/0", "/dev/stdin"),
                getSymLink("/proc/self/fd/1", "/dev/stdout"),
                getSymLink("/proc/self/fd/2", "/dev/stderr")
            };
            return st;
        }
    }

    ContainerConfig::ContainerConfig():
        containersDir("/tmp"),
        lxcConfig(getLXCConfig()),
        processGroupDefaultSettings(getProcessGroupDefaultSettings()),
        controlProcessConfig(getControlProcessConfig()),
        filesystemConfig(getFilesystemConfig())
    {
    }

    ContainerConfig ContainerConfig::fromEnvironment()
    {
        constexpr const char *env = "INVOKER_CONFIG";
        ContainerConfig cfg;
        const char *const cfgPathC = std::getenv(env);
        if (cfgPathC)
        {
            BUNSAN_EXCEPTIONS_WRAP_BEGIN()
            {
                bunsan::filesystem::ifstream fin(cfgPathC);
                fin >> cfg;
                fin.close();
            }
            BUNSAN_EXCEPTIONS_WRAP_END()
        }
        return cfg;
    }

    std::istream &operator>>(std::istream &in, ContainerConfig &config)
    {
        boost::property_tree::ptree cfg;
        try
        {
            boost::property_tree::read_json(in, cfg);
        }
        catch (boost::property_tree::ptree_error &e)
        {
            BOOST_THROW_EXCEPTION(ConfigurationError() << Error::message(e.what()));
        }
        bunsan::config::input_archive<boost::property_tree::ptree>::load_from_ptree(config, cfg);
        return in;
    }

    std::ostream &operator<<(std::ostream &out, const ContainerConfig &config)
    {
        boost::property_tree::ptree cfg;
        bunsan::config::output_archive<boost::property_tree::ptree>::save_to_ptree(config, cfg);
        boost::property_tree::write_json(out, cfg);
        return out;
    }
}}}
