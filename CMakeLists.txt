cmake_minimum_required(VERSION 3.16)

project(frogfs-standalone-demo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CONFIG_FROGFS_USE_ZLIB y)
set(CONFIG_FROGFS_USE_HEATSHRINK y)

include(frogfs/cmake/standalone.cmake)

add_executable(frogfs-standalone-demo
    src/main.c
)
target_link_libraries(frogfs-standalone-demo
    frogfs
)

target_add_frogfs(frogfs-standalone-demo)
