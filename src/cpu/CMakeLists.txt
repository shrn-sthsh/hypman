# Define list of headers and sources
set(HEADERS "")
set(SOURCES "")

# Search subdirectories
add_subdirectory(sys)
add_subdirectory(mod)

# Add entry point file
list(APPEND 
  HEADERS cpuman.hpp
)
list(APPEND 
  SOURCES cpuman.cpp
)

# Create executable
add_executable(
  cpuman ${SOURCES}
)

# Link out of source tree libraries
target_link_libraries(cpuman PRIVATE 
  log
  libvirt ${LIBVIRT_LIBRARIES} 
  signal
)

# Add headers to /include
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/sys
  ${CMAKE_CURRENT_SOURCE_DIR}/mod
)
