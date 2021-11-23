cmake_minimum_required(VERSION 3.20)
project(cbsodata_dumper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(vendor/cpr)

add_executable(cbsodata_dumper main.cpp job_queue.hpp process.cpp process.hpp json.hpp)

target_include_directories(cbsodata_dumper PRIVATE vendor/rapidjson/include)

target_link_libraries(cbsodata_dumper PRIVATE cpr)