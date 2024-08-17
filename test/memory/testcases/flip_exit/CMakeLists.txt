set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

# Create an executable target for the tests
add_executable(flip_exit_domain ${TEST_SOURCES})

# Link the test executable with the logging and statistics libraries
target_link_libraries(flip_exit_domain PRIVATE log stat)

# Enable testing
enable_testing()

# Add the test executable as a test
add_test(NAME flip_exit_domain COMMAND flip_exit_domain)
