# Define list of headers and sources
set(HEADERS "")
set(SOURCES "")

# Search subdirectories
add_subdirectory(sys)
add_subdirectory(util)

# Add entry point file
list(APPEND HEADERS cpuman.hpp)
list(APPEND SOURCES cpuman.cpp)

# Create executable
add_executable(cpuman ${SOURCES})

# Add headers to /include
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sys
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
