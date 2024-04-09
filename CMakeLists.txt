cmake_minimum_required(VERSION 3.20)

project(openal-demo)

set(CMAKE_CXX_STANDARD 20)
# find_library(AL)

aux_source_directory(src SRC)

add_executable(mp3-player ${SRC})

target_link_libraries(mp3-player openal mpg123)


