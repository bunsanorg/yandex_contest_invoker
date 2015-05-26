cmake_minimum_required(VERSION 3.0)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

project(yandex_contest_invoker)

if(BunsanCMakeBuiltin)
    BunsanCMakeLoad()
else()
    find_package(BunsanCMake CONFIG REQUIRED)
endif()

bunsan_add_library(${PROJECT_NAME}
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
    src/lib/detail/execution/AsyncProcessGroup/EventWriter.cpp
    src/lib/detail/execution/AsyncProcessGroup/EventWriter/NativeEventWriter.cpp
    src/lib/detail/execution/AsyncProcessGroup/EventWriter/PlainTextEventWriter.cpp
)
bunsan_use_bunsan_package(${PROJECT_NAME} yandex_contest_common yandex_contest_common)
bunsan_use_bunsan_package(${PROJECT_NAME} yandex_contest_system yandex_contest_system)
bunsan_use_boost(${PROJECT_NAME} thread)

foreach(bin cli ctl)
    bunsan_add_executable(${PROJECT_NAME}_${bin}
        src/bin/${bin}.cpp
    )
    bunsan_use_target(${PROJECT_NAME}_${bin} ${PROJECT_NAME})
    bunsan_install_targets(${PROJECT_NAME}_${bin})
endforeach()

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME})
bunsan_install_project()

bunsan_include_tests()
