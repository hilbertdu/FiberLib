# ==================================================================================================
# CMake
# ==================================================================================================
cmake_minimum_required(VERSION 3.10)

# ==================================================================================================
# Project declaration
# ==================================================================================================
project(FiberLib)

# ==================================================================================================
# Options
# ==================================================================================================
option(LINK_USE_STATIC_CRT 		"Link against the static runtime libraries."	ON)
option(BUILD_UNITESTS			"Build unit-tests" 								OFF)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (${LINK_USE_STATIC_CRT})
	set(CRT_FLAGS_RELEASE "/MT")
	set(CRT_FLAGS_DEBUG "/MTd")
else()
	set(CRT_FLAGS_RELEASE "/MD")
	set(CRT_FLAGS_DEBUG "/MDd")
endif()

# ==================================================================================================
#  Paths
# ==================================================================================================
# Where our application are
set(FIBERLIB ${CMAKE_CURRENT_SOURCE_DIR}/FiberLib)

# Where our unit tests are
set(UNITTESTS ${CMAKE_CURRENT_SOURCE_DIR}/Tests)

# Where our code dir are
set(ROOT_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Where our root dir are
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ==================================================================================================
# Compiler check
# ==================================================================================================
set(MSVC_NATIVE true)

# ==================================================================================================
# General compiler flags
# ==================================================================================================
set(CXX_STANDARD "/std:c++17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /W0 /Zc:__cplusplus")

if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE")
	add_definitions(-D__WINDOWS__)
endif()

# ==================================================================================================
# Debug compiler flags
# ==================================================================================================
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(__DEBUG__ true)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__DEBUG__")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D__DEBUG__")

# ==================================================================================================
# Release compiler flags
# ==================================================================================================

# ==================================================================================================
# Linker flags
# ==================================================================================================

# ==================================================================================================
# Sub-projects
# ==================================================================================================
set(PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${FIBERLIB})

if (BUILD_UNITESTS)
	add_subdirectory(${UNITTESTS})
endif()
