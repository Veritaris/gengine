cmake_minimum_required(VERSION 3.26)
project(gengine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB SOURCES
        "${SRC_DIR}/*.h"
        "${SRC_DIR}/*.cpp"
)

add_definitions(-DRESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources/\")

find_package(SDL2)
if (${SDL2_FOUND})
    message(STATUS "Found SDL2")
    include_directories(${SDL2_INCLUDE_DIRS})
    add_executable(${PROJECT_NAME} ${SOURCES}
            src/network.h
            src/client.c
            src/server.c
            src/server.c)
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
else ()
    message(STATUS "Could not locate SDL2, pizdec")
endif ()