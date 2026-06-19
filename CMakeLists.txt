cmake_minimum_required(VERSION 3.16)
project(plant_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(plant_monitor ${SOURCES})

target_include_directories(plant_monitor PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

find_package(SQLite3 REQUIRED)
target_link_libraries(plant_monitor PRIVATE SQLite::SQLite3)

find_package(Threads REQUIRED)
target_link_libraries(plant_monitor PRIVATE Threads::Threads)

find_package(CURL REQUIRED)
target_link_libraries(plant_monitor PRIVATE CURL::libcurl)

# Some Linux/ARM toolchains require an explicit libatomic link for
# 64-bit atomic builtins, while macOS does not ship that library.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(plant_monitor PRIVATE atomic)
endif()


