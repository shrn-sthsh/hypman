cmake_minimum_required(VERSION 3.10)
project(hypman VERSION 1.0.0)

# C++ options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find libvirt package
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBVIRT REQUIRED libvirt)

# Add out of source tree libraries
add_subdirectory(${CMAKE_SOURCE_DIR}/lib)

# Add source directory
add_subdirectory(${CMAKE_SOURCE_DIR}/src)

# Add testing directory
add_subdirectory(${CMAKE_SOURCE_DIR}/test)
