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
  "Set to ON to build tests for gch::tracker."
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

if (NOT TARGET plf::plf_list)
  find_package (Git REQUIRED)
  execute_process (
    COMMAND
      ${GIT_EXECUTABLE} submodule --quiet update --init -- external/plf_list
    WORKING_DIRECTORY
      ${PROJECT_SOURCE_DIR}
  )

  add_subdirectory (external/plf_list)
endif ()

if (GCH_TRACKER_BUILD_TESTS)
  enable_testing ()
endif ()

add_subdirectory (source)

export (
  EXPORT
    tracker-targets
  NAMESPACE
    gch::
)
