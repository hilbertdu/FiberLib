# ==================================================================================================
# CMake
# ==================================================================================================
cmake_minimum_required(VERSION 3.10)

# ==================================================================================================
# Project declaration
# ==================================================================================================
project(Tests)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(TARGET Tests)

set(PUBLIC_INCLUDE_DIR 
	${ROOT_CODE_DIR}/FiberLib/Includes
)

# ==================================================================================================
# Source declaration
# ==================================================================================================
file(GLOB_RECURSE PROJECT_SOURCE_FILES
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	*.c *.h *.cpp *.hpp *.natvis
)
foreach(source IN LISTS PROJECT_SOURCE_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()  

add_executable(${TARGET} ${PROJECT_SOURCE_FILES})
target_include_directories(${TARGET} PUBLIC ${PUBLIC_INCLUDE_DIR})
target_link_libraries(${TARGET} FiberLib)
