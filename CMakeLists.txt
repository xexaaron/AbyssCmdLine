cmake_minimum_required(VERSION 3.28.3)
project(CmdLine)

set(CMAKE_CXX_STANDARD 23)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CPP_HEADERS Source/Public/CmdLine/CmdLine.h)
set(CPP_SOURCES Source/Private/CmdLine.cpp)
source_group("Public" FILES ${CPP_HEADERS})
source_group("Private" FILES ${CPP_SOURCES})

add_library(${PROJECT_NAME} STATIC ${CPP_HEADERS} ${CPP_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "Source/Public")


