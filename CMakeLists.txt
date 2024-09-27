cmake_minimum_required(VERSION 3.16)
project(MyProject)

# Set the CMP0069 policy, because warning annoying
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()

if(MSVC)
	# Set MSVC runtime library
	add_compile_options(/arch:AVX2) #make sure SIMD optimizations take place

	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) #link time optimization

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(thirdparty/imgui)
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/glfw-3.3.2)

set(DLL_NAME "relaunchDll")
# add dll since we need it
add_subdirectory(dll)

project(MyProgram)

# Change the executable name to exeLocker
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(exeLocker ${MY_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.o")

set_property(TARGET exeLocker PROPERTY CXX_STANDARD 17)

# Include the directory where dll.h is located
include_directories("${CMAKE_SOURCE_DIR}/include")

# Set the output path for the executable to the bin directory
set_target_properties(exeLocker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_definitions(exeLocker PRIVATE DLL_NAME="${DLL_NAME}")
# Pass the DLL path as a definition to the executable
target_compile_definitions(exeLocker PUBLIC GLFW_INCLUDE_NONE=1) 

if(MSVC)
	target_compile_definitions(exeLocker PUBLIC _CRT_SECURE_NO_WARNINGS)
	set_target_properties(exeLocker PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup") #no console
endif()

# Enet not working yet on linux for some reason
target_link_libraries(exeLocker PRIVATE imgui glfw glad)
