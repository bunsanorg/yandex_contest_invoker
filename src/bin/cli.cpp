/*!
 * \file
 *
 * \brief Command line interface to Invoker.
 */

#include <yandex/contest/invoker/All.hpp>
#include <yandex/contest/invoker/detail/VectorToString.hpp>

#include <yandex/contest/StreamLog.hpp>
#include <yandex/contest/TypeInfo.hpp>

#include <bunsan/application.hpp>
#include <bunsan/config/output_archive.hpp>
#include <bunsan/filesystem/fstream.hpp>

#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <iostream>

namespace yandex {
namespace contest {
namespace invoker {
namespace cli {

ContainerConfig parseConfig(const boost::filesystem::path &config) {
  STREAM_INFO << "Trying to load configuration file at " << config << ".";
  ContainerConfig cfg;
  bunsan::filesystem::ifstream cfgin(config);
  BUNSAN_FILESYSTEM_FSTREAM_WRAP_BEGIN(cfgin) {
    cfgin >> cfg;
  } BUNSAN_FILESYSTEM_FSTREAM_WRAP_END(cfgin)
  cfgin.close();
  return cfg;
}

template <typename T>
void printSerializable(std::ostream &out, const T &obj) {
  boost::property_tree::ptree ptree;
  bunsan::config::output_archive<boost::property_tree::ptree> oa(ptree);
  oa << obj;
  boost::property_tree::write_json(out, ptree);
}

using namespace bunsan::application;

class Application : public application {
 public:
  using application::application;

  void initialize_argument_parser(argument_parser &parser) override {
    parser.add_options()(
        "config,c", value<std::string>(&config), "configuration file"
    )(
        "executable,e", value<std::string>(&executable)->required(),
        "executable"
    )(
        "time-limit,t", value<std::uint64_t>(&timeLimitNanos),
        "time limit in nanoseconds"
    )(
        "memory-limit,m", value<std::uint64_t>(&memoryLimitBytes),
        "memory limit in bytes"
    )(
        "output-limit,o", value<std::uint64_t>(&outputLimitBytes),
        "output limit in bytes"
    )(
        "real-time-limit,l", value<std::uint64_t>(&realTimeLimitMillis),
        "real time limit in milliseconds"
    )(
        "stdin", value<std::string>(&inFile)->default_value("/dev/null"),
        "file for stdin"
    )(
        "stdout", value<std::string>(&outFile)->default_value("/dev/null"),
        "file for stdout"
    )(
        "stderr", value<std::string>(&errFile)->default_value("/dev/null"),
        "file for stderr"
    );
    parser.add_positional("argument", -1,
                          value<ProcessArguments>(&arguments)->composing(),
                          "arguments");
  }

  int main(const variables_map &variables) override {
    containerConfig = variables.count("config")
                          ? parseConfig(config)
                          : ContainerConfig::fromEnvironment();

    Process::ResourceLimits processResourceLimits =
        containerConfig.processGroupDefaultSettings.processDefaultSettings
            .resourceLimits;

    if (variables.count("time-limit"))
      processResourceLimits.timeLimit =
          std::chrono::nanoseconds(timeLimitNanos);

    if (variables.count("memory-limit"))
      processResourceLimits.memoryLimitBytes = memoryLimitBytes;

    if (variables.count("output-limit"))
      processResourceLimits.outputLimitBytes = outputLimitBytes;

    ProcessGroup::ResourceLimits processGroupResourceLimits =
        containerConfig.processGroupDefaultSettings.resourceLimits;

    if (variables.count("real-time-limit"))
      processGroupResourceLimits.realTimeLimit =
          std::chrono::milliseconds(realTimeLimitMillis);

    execute();

    return exit_success;
  }

  void execute() {
    STREAM_INFO << "Trying to execute " << executable << " with "
                << detail::vectorToString(arguments) << " arguments "
                << "where " << STREAM_OBJECT(processResourceLimits);

    ContainerPointer container = Container::create(containerConfig);
    ProcessGroupPointer processGroup = container->createProcessGroup();
    processGroup->setResourceLimits(processGroupResourceLimits);
    ProcessPointer process = processGroup->createProcess(executable);
    process->setArguments(arguments);
    process->setResourceLimits(processResourceLimits);
    if (inFile != "/dev/null") {
      container->filesystem().push(inFile, "/stdin",
                                   system::unistd::access::Id(0, 0), 0400);
      process->setStream(0, File("/stdin", AccessMode::READ_ONLY));
    }
    if (outFile != "/dev/null")
      process->setStream(1, File("/stdout", AccessMode::WRITE_ONLY));
    if (errFile != "/dev/null")
      process->setStream(2, File("/stderr", AccessMode::WRITE_ONLY));
    const ProcessGroup::Result processGroupResult =
        processGroup->synchronizedCall();
    const Process::Result processResult = process->result();
    STREAM_INFO << "Process group has terminated";
    // output results
    std::cout << "Process group result:" << std::endl;
    printSerializable(std::cout, processGroupResult);
    std::cout << "Process result:" << std::endl;
    printSerializable(std::cout, processResult);
    if (outFile != "/dev/null")
      container->filesystem().pull("/stdout", outFile);
    if (errFile != "/dev/null")
      container->filesystem().pull("/stderr", errFile);
  }

 private:
  std::string config, executable, inFile, outFile, errFile;
  std::uint64_t timeLimitNanos;
  std::uint64_t memoryLimitBytes;
  std::uint64_t outputLimitBytes;
  std::uint64_t realTimeLimitMillis;
  ProcessArguments arguments;
  ContainerConfig containerConfig;
  ProcessGroup::ResourceLimits processGroupResourceLimits;
  Process::ResourceLimits processResourceLimits;
};

}  // namespace cli
}  // namespace invoker
}  // namespace contest
}  // namespace yandex

int main(int argc, char *argv[]) {
  yandex::contest::invoker::cli::Application app(argc, argv);
  app.name("yandex::contest::invoker::cli");
  return app.exec();
}
