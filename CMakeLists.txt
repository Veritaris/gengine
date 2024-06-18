cmake_minimum_required(VERSION 3.26)
project(gengine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_definitions(-DRESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources/\")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")

include_directories(${SRC_DIR}/include)

add_subdirectory(unicode)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

link_libraries(
    SDL3::SDL3
    unicode
)

add_library(c-client STATIC
    src/client.c
)

add_library(c-server STATIC
    src/server.c
)

add_library(array_list STATIC
    src/array_list.c
)

add_executable(${PROJECT_NAME}-client
    src/game_client.cpp
)

add_executable(${PROJECT_NAME}-server
    src/server.c
)

target_compile_definitions(${PROJECT_NAME}-client
    PRIVATE RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

target_link_libraries(c-client)
target_link_libraries(c-server
    INTERFACE SDL3::SDL3
)
target_link_libraries(${PROJECT_NAME}-client)
target_link_libraries(${PROJECT_NAME}-server
    PRIVATE array_list
)