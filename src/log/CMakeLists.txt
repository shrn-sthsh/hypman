# Add source as library
add_library(log record.cpp)

# Add header in include
target_include_directories(log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
