set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

# Create an executable target for the tests
add_executable(all_on_one_cpu ${TEST_SOURCES})

# Link the test executable with the logging and statistics libraries
target_link_libraries(all_on_one_cpu PRIVATE log stat)

# Enable testing
enable_testing()

# Add the test executable as a test
add_test(NAME all_on_one_cpu COMMAND all_on_one_cpu)
