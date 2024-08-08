# Add headers as interface libraries
add_library(stat INTERFACE)

# Specify include directories for the interface library
target_include_directories(stat INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
