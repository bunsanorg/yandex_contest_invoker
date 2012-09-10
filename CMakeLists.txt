project(yandex_contest_invoker)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(YandexContestCommon)

# BOOST
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS system filesystem serialization unit_test_framework program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND libraries ${Boost_LIBRARIES})

# yandex_contest_*
list(APPEND libraries
    yandex_contest_common
    yandex_contest_system
)

# Local
include_directories(include)

# Targets

add_library(${PROJECT_NAME} SHARED
    src/lib/Container.cpp
    src/lib/Filesystem.cpp
    src/lib/ProcessGroup.cpp
    src/lib/Process.cpp
    src/lib/ContainerConfig.cpp
    src/lib/ControlProcessConfig.cpp
    src/lib/process_group/DefaultSettings.cpp
    src/lib/process/Result.cpp
    src/lib/process/DefaultSettings.cpp
    src/lib/filesystem/File.cpp
    src/lib/filesystem/RegularFile.cpp
    src/lib/filesystem/Device.cpp
    src/lib/filesystem/Directory.cpp
    src/lib/filesystem/SymLink.cpp
    src/lib/filesystem/FIFO.cpp
    src/lib/filesystem/CreateFile.cpp
    src/lib/filesystem/Operations.cpp
    src/lib/detail/execution/AsyncProcessGroup.cpp
    src/lib/detail/execution/AsyncProcessGroup/detail.cpp
    src/lib/detail/execution/AsyncProcessGroup/execute.cpp
    src/lib/detail/execution/AsyncProcessGroup/ProcessGroupStarter.cpp
    src/lib/detail/execution/AsyncProcessGroup/ProcessStarter.cpp
    src/lib/detail/execution/AsyncProcessGroup/ExecutionMonitor.cpp
    src/lib/detail/execution/AsyncProcessGroup/Streams.cpp
)
target_link_libraries(${PROJECT_NAME} ${libraries})

set(bin_srcs cli ctl)
set(bin_targets)

foreach(bin ${bin_srcs})
    set(trgt ${PROJECT_NAME}_${bin})
    list(APPEND bin_targets ${trgt})
    set(binprefix src/bin/${bin})
    if(EXISTS ${CMAKE_SOURCE_DIR}/${binprefix}.cpp)
        add_executable(${trgt} ${binprefix}.cpp)
    elseif(EXISTS ${CMAKE_SOURCE_DIR}/${binprefix}.c)
        add_executable(${trgt} ${binprefix}.c)
    else()
        message(SEND_ERROR "Source does not exist for target ${trgt}")
    endif()
    target_link_libraries(${trgt} ${PROJECT_NAME} ${libraries})
endforeach()

# installation
install(DIRECTORY include DESTINATION .)
install(TARGETS ${PROJECT_NAME} ${bin_targets}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

yandex_contest_tests()
