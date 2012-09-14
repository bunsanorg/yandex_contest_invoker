project(${PROJECT_NAME}_tests_resources)

add_subdirectory(benchmark ${CMAKE_CURRENT_BINARY_DIR}/benchmark)

add_executable(exec exec.c burn_cpu.c)
