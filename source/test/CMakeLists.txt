SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ADD_EXECUTABLE(test_tracker main.cpp)
TARGET_LINK_LIBRARIES(test_tracker tracker)

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

SET_TARGET_PROPERTIES(test_tracker PROPERTIES
                      CXX_STANDARD          20
                      CXX_STANDARD_REQUIRED NO
                      CXX_EXTENSIONS        NO
                      COMPILE_OPTIONS       -fno-rtti)
