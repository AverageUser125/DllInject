project(MyDLL)

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