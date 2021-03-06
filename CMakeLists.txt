cmake_minimum_required(VERSION 3.6)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(net4cxx)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_SKIP_BUILD_RPATH 0)
set(CMAKE_BUILD_WITH_INSTALL_RPATH 0)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

include(CheckCXXSourceRuns)
include(CheckIncludeFiles)

include(CheckPlatform)
include(AutoCollect)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
message(STATUS ${CMAKE_CXX_COMPILER_ID})
message(STATUS ${CMAKE_CXX_COMPILER_VERSION})
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.0")
        message(STATUS "GCC < 4.8 doesn't support the address sanitizer")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    endif()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.0")
        message(STATUS "GCC < 4.9 doesn't support the undefined behavior sanitizer")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
#    if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
#        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error")
#    else()
#        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#    endif()
endif()

add_subdirectory(dep)
add_subdirectory(src)
#add_subdirectory(test)
add_subdirectory(example)
