set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

# Create an executable target for the tests
add_executable(even_split_domain ${TEST_SOURCES})

# Enable testing
enable_testing()

# Add the test executable as a test
add_test(NAME even_split_domain COMMAND even_split_domain)
