if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  string (REGEX REPLACE "/[wW]([0-4deovX]|all) ?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string (REGEX REPLACE "/GR ?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif ()

macro (add_unit_test target_name)
  add_executable (${target_name} ${ARGN})
  target_link_libraries (${target_name} PRIVATE gch::tracker)

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options (
      ${target_name}
      PRIVATE
        -fno-rtti
        -ftime-trace
        --coverage
        -Weverything
        -Wno-padded
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-zero-as-null-pointer-constant
        -Wno-used-but-marked-unused
    )

    target_link_options (
      ${target_name}
      PRIVATE
        -fno-rtti
        --coverage
    )

    if (GCH_USE_LIBCXX_WITH_CLANG)
      target_compile_options (${target_name} PRIVATE -stdlib=libc++)
      target_link_options (${target_name} PRIVATE LINKER:--allow-multiple-definition)
      target_link_libraries (${target_name} PRIVATE c++ c++abi)
    endif ()
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options (
      ${target_name}
      PRIVATE
        -fno-rtti
        -pedantic
        -Wall
        -Wextra
        -Wcast-align
        -Wcast-qual
        -Wctor-dtor-privacy
        -Wdisabled-optimization
        -Wformat=2
        -Winit-self
        -Wlogical-op
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wnoexcept
        -Wold-style-cast
        -Woverloaded-virtual
        -Wredundant-decls
        -Wshadow
        -Wsign-conversion
        -Wsign-promo
        -Wstrict-null-sentinel
        -Wstrict-overflow=5
        -Wswitch-default
        -Wundef
        -Wno-unused
    )
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options (
      ${target_name}
      PRIVATE
        /W4
        /GR-
    )
  endif ()

  if (UNIX)
    target_compile_options (${target_name} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
    target_link_options (${target_name} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
  endif ()

  add_test (
    NAME
      ${target_name}
    COMMAND
      ${target_name}
  )
endmacro ()

set (TRACKER_TEST_NAMES
     main
     )

foreach (version 11 14 17 20)
  foreach (name ${TRACKER_TEST_NAMES})
    add_unit_test (tracker.${name}.c++${version} ${name}.cpp)

    set_target_properties (
      tracker.${name}.c++${version}
      PROPERTIES
      CXX_STANDARD
        ${version}
      CXX_STANDARD_REQUIRED
        NO
      CXX_EXTENSIONS
        NO
    )
  endforeach ()
endforeach ()
