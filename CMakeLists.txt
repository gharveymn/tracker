CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(tracker
        VERSION 0.0.1
        LANGUAGES CXX)

INCLUDE(cmake/create-package.cmake)

CREATE_HEADER_ONLY_PACKAGE(AUTHOR       gch
                           NAME         tracker
                           DEPENDENCIES optional_ref plf_list
                           HEADERS      tracker.hpp)

# Options
OPTION(TRACKER_BUILD_TESTS "Set to ON to build tests" ON)

# Only include tests if we are at the top level
IF(TRACKER_BUILD_TESTS AND (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
  ADD_SUBDIRECTORY(test)
ENDIF()
