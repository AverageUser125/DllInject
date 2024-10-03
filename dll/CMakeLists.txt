cmake_minimum_required(VERSION 3.16)

set(DLL_NAME "relaunchDll")

project(${DLL_NAME})

if(MSVC)
	# Set MSVC runtime library
	add_compile_options(/arch:AVX2) #make sure SIMD optimizations take place

	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(${DLL_NAME} SHARED dll.cpp dll.h)
target_compile_definitions(${DLL_NAME} PRIVATE DLL_NAME="${DLL_NAME}")

set_property(TARGET ${DLL_NAME} PROPERTY CXX_STANDARD 17)

set_target_properties(${DLL_NAME} PROPERTIES PREFIX "")

# Set the output path for the DLL
set_target_properties(${DLL_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
set_target_properties(${DLL_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)