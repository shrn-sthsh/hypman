# Add utilities' source directory
add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/util/log
)

# Add cpuman and memoryman source directories
add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/cpu
)
add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/memory
)
