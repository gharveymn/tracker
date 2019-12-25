IF (TRACKER_BUILD_TESTS)

  ADD_EXECUTABLE(test main.cpp)
  TARGET_LINK_LIBRARIES(test tracker)

  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
  endif()

  if (UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
  endif()

  SET_TARGET_PROPERTIES(test PROPERTIES
                        CXX_STANDARD          17
                        CXX_STANDARD_REQUIRED NO
                        CXX_EXTENSIONS        NO)

ENDIF()
