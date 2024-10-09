cmake_minimum_required(VERSION 3.10)
project(injector)

# Check if the platform is Windows
if(NOT WIN32)
    # Specify the path to the existing static library
    set(INJECTOR_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libinjector.a")

    # Create an interface library to hold the dependency
    add_library(injector_static INTERFACE)

    # Specify the location of the existing library
    target_link_libraries(injector_static INTERFACE ${INJECTOR_LIB_PATH})

    set_target_properties(injector_static PROPERTIES LINK_FLAGS "-Wl,-z,nx")

    # Include directories
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
    target_include_directories(injector_static INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

    # This is now done in the target that uses injector_static
endif()