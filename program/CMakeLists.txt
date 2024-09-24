project(MyProgram)

add_executable(MyProgram main.cpp)

set_property(TARGET MyProgram PROPERTY CXX_STANDARD 17)

# Include the directory where dll.h is located
include_directories("${CMAKE_BINARY_DIR}/include")

# Set the output path for the executable to the bin directory
set_target_properties(MyProgram PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Set DLL_PATH based on the PRODUCTION option
set(DLL_PATH "./MyDLL.dll")

# Pass the DLL path as a definition to the executable
target_compile_definitions(MyProgram PRIVATE DLL_PATH=\"${DLL_PATH}\")


#enet not working yet on linux for some reason
target_link_libraries(MyProgram PRIVATE imgui)