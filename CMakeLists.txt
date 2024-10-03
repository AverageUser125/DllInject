cmake_minimum_required(VERSION 3.10)

# project name
set(PROJECT_NAME "exeLocker")

# Set the Compiler
# set(CMAKE_CXX_COMPILER /usr/bin/g++)

# set to C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

# Set the CMP0069 policy, because warning annoying
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND TRUE CACHE BOOL "" FORCE)

# /Zi and /GL are incompatible, so here
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)	
endif()

if(MSVC)
    # Set MSVC runtime library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Enable SIMD optimizations
    add_compile_options(/arch:AVX2)

    # Disable CRT secure warnings
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # Show all warnings, just nice to have
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-sign-compare -Wno-unused -Wno-delete-incomplete")
endif()

project("${PROJECT_NAME}")

# Add source files recursively from the src directory
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Add source to this project's executable
add_executable(${PROJECT_NAME} ${MY_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# Set the output path for the executable to the bin directory
set_target_properties(exeLocker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_subdirectory(thirdparty/imgui)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/injector)

set(DLL_NAME "relaunchDll")
add_subdirectory(dll)

target_compile_definitions(exeLocker PRIVATE DLL_NAME="${DLL_NAME}")
if(MSVC)
    # Set linker flags for Windows subsystem and entry point
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

# add .h and .hpp files
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw imgui injector_static)
else()
    add_subdirectory(thirdparty/stb_image)
    target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw imgui stb_image injector_static)
endif()


# Specify the output directory for the build (/bin directory)
# set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
# Specify the output directory for the build (/build/bin directory)
# set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")