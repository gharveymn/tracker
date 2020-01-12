EXECUTE_PROCESS(COMMAND "git submodule update --init -- \"external/create-package\""
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

INCLUDE("${PROJECT_SOURCE_DIR}/external/create-package/cmake/create-package.cmake" OPTIONAL)

CREATE_PACKAGE(AUTHOR       gharveymn
               DEPENDENCIES optional_ref plf_list
               HEADERS      tracker.hpp)

# Options
IF(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  OPTION(TRACKER_BUILD_TESTS "Set to ON to build tests" ON)
ELSE()
  OPTION(TRACKER_BUILD_TESTS "Set to ON to build tests" OFF)
ENDIF()

IF(TRACKER_BUILD_TESTS)
  ADD_SUBDIRECTORY(test)
ENDIF()
