cmake_minimum_required(VERSION 3.16)

project(MyDLL)

include(CheckIPOSupported)
check_ipo_supported()

if(MSVC)
	# Set MSVC runtime library
	add_compile_options(/arch:AVX2) #make sure SIMD optimizations take place

	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(MyDLL SHARED dll.cpp dll.h)

set_property(TARGET MyDLL PROPERTY CXX_STANDARD 17)

# Set the output path for the DLL
set_target_properties(MyDLL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install the DLL and its headers for other projects
install(TARGETS MyDLL
    RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/bin"
)