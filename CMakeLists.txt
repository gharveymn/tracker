cmake_minimum_required (VERSION 3.15)

project (
  tracker
  VERSION
    0.0.3
  LANGUAGES
    CXX
)

string (COMPARE EQUAL "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}" _EXTRAS_DEFAULT)

option (
  GCH_TRACKER_BUILD_TESTS
  "Set to ON to build tests for gch::small_vector."
  ${_EXTRAS_DEFAULT}
)

include (CMakeDependentOption)
cmake_dependent_option (
  GCH_USE_LIBCXX_WITH_CLANG
  "Set to ON to use libc++ when compiling with Clang."
  OFF
  "CMAKE_CXX_COMPILER_ID MATCHES Clang"
  OFF
)

if (GCH_TRACKER_BUILD_TESTS)
  enable_testing ()
endif ()

find_package (Git REQUIRED)
execute_process (
  COMMAND
    ${GIT_EXECUTABLE} submodule --quiet update --init -- external/plf_list
  WORKING_DIRECTORY
    ${PROJECT_SOURCE_DIR}
)

add_subdirectory (external/plf_list EXCLUDE_FROM_ALL)

add_library (plf::plf_list INTERFACE IMPORTED)
target_include_directories (plf::plf_list INTERFACE ${PROJECT_SOURCE_DIR}/external/plf_list)

add_subdirectory (source)

export (
  EXPORT
    tracker-targets
  NAMESPACE
    gch::
)
