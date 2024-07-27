# Add headers as interface libraries
add_library(libvirt INTERFACE)
add_library(signal  INTERFACE)

# Specify include directories for the interface library
target_include_directories(libvirt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libvirt)
target_include_directories(signal  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/signal)
