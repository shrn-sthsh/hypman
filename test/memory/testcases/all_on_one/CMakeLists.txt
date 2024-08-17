set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

# Create an executable target for the tests
add_executable(all_on_one_domain ${TEST_SOURCES})

# Enable testing
enable_testing()

# Add the test executable as a test
add_test(NAME all_on_one_domain COMMAND all_on_one_domain)
