cmake_minimum_required(VERSION 2.8.7)

project(VideoSynthesis C CXX)

# Developing version
set(VIDEOSYNTHESIS_TARGET_VERSION "0.0.1" CACHE STRING "Video Synthesis Development version")
add_definitions(-DVIDEOSYSTHESIS_VERSION=${VIDEOSYNTHESIS_TARGET_VERSION})

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Default build type: Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Add visibility of headers
file( GLOB_RECURSE MAKE_HEADERS_VISIBLE_SRC *.cpp *.hpp *.h)
add_custom_target( MAKE_HEADERS_VISIBLE SOURCES ${MAKE_HEADERS_VISIBLE_SRC})

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wshadow -Wno-sign-compare")
if(${CMAKE_BUILD_TYPE} EQUAL "Debug")
  add_definitions(-D_DEBUG)
elseif(${CMAKE_BUILD_TYPE} EQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuinitialized")
endif()
 
#Settings
set(M32 false CACHE BOOL "Build to M32 or X64")
if(${M32})
  message(STATUS "Buiding to M32")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()

set(HIGHBITDEPTH false CACHE BOOL "Selct the high bit depth")
if(${HIGHBITDEPTH})
  message(STATUS "Using high bit depth")
  add_definitions(-DExt__HIGH_BIT_DEPTH_SUPPORT=1)
endif

add_subdirectory(source)
