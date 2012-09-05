/*!
 * \file
 *
 * \brief Command line interface to Invoker.
 */

#include "yandex/contest/invoker/All.hpp"

#include "yandex/contest/detail/LogHelper.hpp"

#include "yandex/contest/SystemError.hpp"

#include "yandex/contest/invoker/detail/VectorToString.hpp"

#include "yandex/contest/config/OutputArchive.hpp"

#include <iostream>
#include <typeinfo>

#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// boost::units::detail::demangle
#include <boost/units/detail/utility.hpp>

namespace yandex{namespace contest{namespace invoker{namespace cli
{
    ContainerConfig parseConfig(const boost::filesystem::path &config)
    {
        STREAM_INFO << "Trying to load configuration file at " << config << ".";
        ContainerConfig cfg;
        boost::filesystem::ifstream cfgin(config);
        if (!cfgin)
            BOOST_THROW_EXCEPTION(SystemError("open"));
        cfgin >> cfg;
        cfgin.close();
        if (!cfgin)
            BOOST_THROW_EXCEPTION(SystemError("close"));
        return cfg;
    }

    template <typename T>
    void printSerializable(std::ostream &out, const T &obj)
    {
        boost::property_tree::ptree ptree;
        config::OutputArchive<boost::property_tree::ptree> oa(ptree);
        oa << obj;
        boost::property_tree::write_json(out, ptree);
    }

    void execute(const ContainerConfig &config,
                 const boost::filesystem::path &executable,
                 const ProcessArguments &arguments,
                 const ProcessGroup::ResourceLimits &processGroupResourceLimits,
                 const Process::ResourceLimits &processResourceLimits)
    {
        STREAM_INFO <<
            "Trying to execute " << executable << " with " <<
            detail::vectorToString(arguments) << " arguments where " <<
            "memory limit = " << processResourceLimits.memoryLimitBytes << " bytes, " <<
            "hard memory limit = " << processResourceLimits.hardMemoryLimitBytes << " bytes, " <<
            "time limit = " << processResourceLimits.timeLimitMillis << " milliseconds, " <<
            "output limit = " << processResourceLimits.outputLimitBytes << " bytes, " <<
            "real time limit = " << processGroupResourceLimits.realTimeLimitMillis << " milliseconds";

        ContainerPointer container = Container::create(config);
        ProcessGroupPointer processGroup = container->createProcessGroup();
        processGroup->setResourceLimits(processGroupResourceLimits);
        ProcessPointer process = processGroup->createProcess(executable);
        process->setArguments(arguments);
        process->setResourceLimits(processResourceLimits);
        const ProcessGroup::Result processGroupResult = processGroup->synchronizedCall();
        const Process::Result processResult = process->result();
        STREAM_INFO << "Process group has terminated";
        // output results
        std::cout << "Process group result:" << std::endl;
        printSerializable(std::cout, processGroupResult);
        std::cout << "Process result:" << std::endl;
        printSerializable(std::cout, processResult);
    }
}}}}

int main(int argc, char *argv[])
{
    namespace ya = yandex::contest::invoker;
    namespace po = boost::program_options;
    po::options_description desc("Usage");
    try
    {
        std::string config, executable;
        ya::ProcessGroup::ResourceLimits processGroupResourceLimits;
        ya::Process::ResourceLimits processResourceLimits;
        ya::ProcessArguments arguments;
        desc.add_options()
            ("config,c", po::value<std::string>(&config), "configuration file")
            ("executable,e", po::value<std::string>(&executable)->required(), "executable")
            ("time-limit,t", po::value<std::uint64_t>(&processResourceLimits.timeLimitMillis),
                "time limit in milliseconds")
            ("memory-limit,m", po::value<std::uint64_t>(&processResourceLimits.memoryLimitBytes),
                "memory limit in bytes")
            ("hard-memory-limit,h", po::value<std::uint64_t>(&processResourceLimits.hardMemoryLimitBytes),
                "hard memory limit in bytes")
            ("output-limit,o", po::value<std::uint64_t>(&processResourceLimits.outputLimitBytes),
                "output limit in bytes")
            ("real-time-limit,l", po::value<std::uint64_t>(&processGroupResourceLimits.realTimeLimitMillis),
                "real time limit in milliseconds")
            ("argument,a", po::value<ya::ProcessArguments>(&arguments)->composing(), "arguments");
        po::positional_options_description pdesc;
        pdesc.add("argument", -1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pdesc).run(), vm);
        po::notify(vm);

        const ya::ContainerConfig cfg = vm.count("config") ?
            ya::cli::parseConfig(config) :
            ya::ContainerConfig::fromEnvironment();
        ya::cli::execute(cfg, executable, arguments,
                         processGroupResourceLimits,
                         processResourceLimits);
    }
    catch (po::error &e)
    {
        std::cerr << e.what() << std::endl << std::endl << desc << std::endl;
        return 200;
    }
    catch (std::exception &e)
    {
        std::cerr << "Program terminated due to exception of type \"" <<
                     boost::units::detail::demangle(typeid(e).name()) << "\"." << std::endl;
        std::cerr << "what() returns the following message:" << std::endl <<
                     e.what() << std::endl;
        return 1;
    }
}
