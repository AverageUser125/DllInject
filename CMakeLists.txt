cmake_minimum_required(VERSION 3.16)
project(MyProject)

if(MSVC)
	# Set MSVC runtime library
	add_compile_options(/arch:AVX2) #make sure SIMD optimizations take place

	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) #link time optimization

# Add subdirectories for the two projects
add_subdirectory("dll")
add_subdirectory("program")
