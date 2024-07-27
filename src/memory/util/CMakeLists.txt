# Define local headers & sources
set(SYSTEM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/domain.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/hardware.hpp
)
set(SYSTEM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/domain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/hardware.cpp
)

# Add local sources and headers to global sources and headers
list(APPEND HEADERS ${SYSTEM_HEADERS})
set(SOURCES ${HEADERS} PARENT_SCOPE)

list(APPEND SOURCES ${SYSTEM_SOURCES})
set(SOURCES ${SOURCES} PARENT_SCOPE)
