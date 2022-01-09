add_library (tracker INTERFACE)

target_link_libraries (tracker INTERFACE $<BUILD_INTERFACE:plf::plf_list>)

target_sources (
  tracker
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/gch/tracker.hpp>
)

target_include_directories (
  tracker
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

target_include_directories (
  tracker
  SYSTEM INTERFACE
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>
)

set_target_properties (
  tracker
  PROPERTIES
  PUBLIC_HEADER
    include/gch/tracker.hpp
)

add_library (gch::tracker ALIAS tracker)

install (
  TARGETS
    tracker
  EXPORT
    tracker-targets
  PUBLIC_HEADER
    DESTINATION include/gch
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/tracker-config.cmake [=[
get_filename_component (PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
include ("${CMAKE_CURRENT_LIST_DIR}/tracker-targets.cmake")
]=])

include (CMakePackageConfigHelpers)
write_basic_package_version_file (
  tracker-version.cmake
  COMPATIBILITY
    SameMinorVersion
)

install (
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/tracker-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/tracker-version.cmake
  DESTINATION
    lib/cmake/tracker
)

install (
  EXPORT
    tracker-targets
  DESTINATION
    lib/cmake/tracker
  NAMESPACE
    gch::
)

install (
  FILES
    ${PROJECT_SOURCE_DIR}/external/plf_list/plf_list.h
  DESTINATION
    include/plf
)

target_include_directories (
  tracker
  SYSTEM INTERFACE
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/plf>
)

# Only include tests if we are at the top level
if (GCH_TRACKER_BUILD_TESTS)
  add_subdirectory (test)
endif ()
