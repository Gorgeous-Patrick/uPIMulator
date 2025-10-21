cmake_minimum_required(VERSION 3.10)
project(host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(/opt/upmem/include/dpu)
link_directories(/opt/upmem/lib)

add_executable(host app.c)
target_link_libraries(host dpu)
