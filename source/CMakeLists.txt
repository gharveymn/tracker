EXECUTE_PROCESS(COMMAND "git submodule update --init -- \"external/create-package\""
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

INCLUDE("${PROJECT_SOURCE_DIR}/external/create-package/cmake/create-package.cmake" OPTIONAL)

CREATE_PACKAGE(AUTHOR       gharveymn
               DEPENDENCIES optional_ref plf_list
               HEADERS      tracker.hpp)

IF(TRACKER_BUILD_TESTS)
  MESSAGE(STATUS "Testing is enabled.")
  ADD_SUBDIRECTORY(test)
ELSE()
  MESSAGE(STATUS "Testing is disabled.")
ENDIF()

