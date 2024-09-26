cmake_minimum_required(VERSION 3.16)

set(DLL_NAME "relaunchDll")

project(${DLL_NAME})

include(CheckIPOSupported)
check_ipo_supported()

if(MSVC)
	# Set MSVC runtime library
	add_compile_options(/arch:AVX2) #make sure SIMD optimizations take place

	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(${DLL_NAME} SHARED dll.cpp dll.h)
target_compile_definitions(${DLL_NAME} PRIVATE DLL_NAME="${DLL_NAME}")

set_property(TARGET ${DLL_NAME} PROPERTY CXX_STANDARD 17)

# Set the output path for the DLL
set_target_properties(${DLL_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install the DLL and its headers for other projects
install(TARGETS ${DLL_NAME}
    RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/bin"
)