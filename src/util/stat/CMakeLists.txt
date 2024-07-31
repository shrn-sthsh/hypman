# Define local headers & sources
set(STAT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/statistics.cpp
)

# Create the library from the source files
add_library(
  stat STATIC ${STAT_SOURCES}
)

# Add headers to includes
target_include_directories(
  stat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)
