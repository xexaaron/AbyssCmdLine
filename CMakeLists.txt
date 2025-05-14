cmake_minimum_required(VERSION 3.28.3)
project(CmdLine)

set(CMAKE_CXX_STANDARD 23)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()


set(ABY_PP_DIR "Vendor/AbyssPrettyPrint")
add_subdirectory(${ABY_PP_DIR})
set(ABY_PP_INCLUDE_DIR "${ABY_PP_DIR}/Source/Public")

set(CPP_HEADERS Source/Public/CmdLine/CmdLine.h)
set(CPP_SOURCES Source/Private/CmdLine.cpp)
source_group("Public" FILES ${CPP_HEADERS})
source_group("Private" FILES ${CPP_SOURCES})


add_library(${PROJECT_NAME} STATIC ${CPP_HEADERS} ${CPP_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "Source/Public" ${ABY_PP_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PrettyPrint)
add_dependencies(${PROJECT_NAME} PrettyPrint)

add_executable(${PROJECT_NAME}Test "Source/Tests/Test.cpp")
target_include_directories(${PROJECT_NAME}Test PRIVATE "Source/Public")
target_link_libraries(${PROJECT_NAME}Test ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME}Test ${PROJECT_NAME})