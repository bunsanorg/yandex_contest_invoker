project(${PROJECT_NAME}_tests_resources)

add_subdirectory(benchmark ${CMAKE_CURRENT_BINARY_DIR}/benchmark)

add_executable(exec exec.c burn_cpu.c)

bunsan_add_executable(listener listener.cpp)
bunsan_use_target(listener yandex_contest_invoker)
bunsan_targets_finish_setup(listener)
