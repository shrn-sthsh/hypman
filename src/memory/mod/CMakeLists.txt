# Define local headers & sources
set(MODULE_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/domain/domain.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hardware/hardware.hpp
)
set(MODULE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/domain/domain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hardware/hardware.cpp
)

# Add local sources and headers to global sources and headers
list(APPEND 
  HEADERS ${MODULE_HEADERS}
)
set(HEADERS 
  ${HEADERS} PARENT_SCOPE
)

list(APPEND 
  SOURCES ${MODULE_SOURCES}
)
set(SOURCES 
  ${SOURCES} PARENT_SCOPE
)
