cmake_minimum_required(VERSION 3.12)
project(log_server)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost)
find_package(Threads)

add_compile_options(-Werror -Wall -Wextra -Wpedantic)

if ("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

add_executable(log_server main.cpp)
target_link_libraries(log_server ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(log_server PRIVATE ${Boost_INCLUDE_DIRS})
