project(yandex_contest_invoker)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(BunsanCommon)

bunsan_use_boost(system filesystem serialization thread program_options)

bunsan_use_bunsan(common)
bunsan_use(
    yandex_contest_common
    yandex_contest_system
)

bunsan_add_shared_library(${PROJECT_NAME}
    src/lib/Container.cpp
    src/lib/Filesystem.cpp
    src/lib/ProcessGroup.cpp
    src/lib/Process.cpp
    src/lib/ContainerConfig.cpp
    src/lib/ControlProcessConfig.cpp
    src/lib/Notifier.cpp
    src/lib/process_group/DefaultSettings.cpp
    src/lib/process/Result.cpp
    src/lib/process/DefaultSettings.cpp
    src/lib/filesystem/File.cpp
    src/lib/filesystem/RegularFile.cpp
    src/lib/filesystem/Device.cpp
    src/lib/filesystem/Directory.cpp
    src/lib/filesystem/SymLink.cpp
    src/lib/filesystem/Fifo.cpp
    src/lib/filesystem/CreateFile.cpp
    src/lib/filesystem/Operations.cpp
    src/lib/notifier/QueuedWriter.cpp
    src/lib/detail/execution/AsyncProcessGroup.cpp
    src/lib/detail/execution/AsyncProcessGroup/detail.cpp
    src/lib/detail/execution/AsyncProcessGroup/execute.cpp
    src/lib/detail/execution/AsyncProcessGroup/ProcessGroupStarter.cpp
    src/lib/detail/execution/AsyncProcessGroup/ProcessStarter.cpp
    src/lib/detail/execution/AsyncProcessGroup/ProcessInfo.cpp
    src/lib/detail/execution/AsyncProcessGroup/ExecutionMonitor.cpp
    src/lib/detail/execution/AsyncProcessGroup/Streams.cpp
    src/lib/detail/execution/AsyncProcessGroup/Notifier.cpp
)
bunsan_use(${PROJECT_NAME})

bunsan_add_cli_targets(cli_targets cli ctl)

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME} ${cli_targets})

bunsan_include_tests()
