project(ping)
cmake_minimum_required(VERSION 2.8)

add_executable(ping main.c)
target_link_libraries(ping libutil)

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_BIN_DIR}/src)
add_subdirectory(include)
add_subdirectory(src)