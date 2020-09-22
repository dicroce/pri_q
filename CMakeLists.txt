cmake_minimum_required (VERSION 3.11.2)
project(pri)

include(FetchContent)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Debug)
endif()

set (CMAKE_CXX_STANDARD 14)

# Dependencies

FetchContent_Declare(
    cppkit
    GIT_REPOSITORY https://github.com/dicroce/cppkit
    GIT_TAG master
)

FetchContent_GetProperties(cppkit)
if(NOT cppkit_POPULATED)
  FetchContent_Populate(cppkit)
  add_subdirectory(${cppkit_SOURCE_DIR} ${cppkit_BINARY_DIR})
endif()

add_library(pri include/pri/pager.h
                   include/pri/pri_q.h
                   source/pager.cpp)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pri PUBLIC "-O0")
    target_compile_options(pri PUBLIC "-g")
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pri PUBLIC "-O3")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

target_include_directories(pri PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pri cppkit)

add_subdirectory(ut)
