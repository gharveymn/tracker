SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ADD_EXECUTABLE(test_tracker main.cpp)
TARGET_LINK_LIBRARIES(test_tracker tracker)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  SET(COMPILE_OPTS -fno-rtti --coverage -Wall -Wextra)
  SET(LINK_OPTS --coverage)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  SET(COMPILE_OPTS -fno-rtti -Wall -Wextra)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  SET(COMPILE_OPTS /GR- /W4)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
      
endif()

if (UNIX)
  LIST(APPEND COMPILE_OPTS -fsanitize=address)
  LIST(APPEND LINK_OPTS -fsanitize=address)
endif()

SET_TARGET_PROPERTIES(test_tracker PROPERTIES
                      CXX_STANDARD          20
                      CXX_STANDARD_REQUIRED NO
                      CXX_EXTENSIONS        NO
                      COMPILE_OPTIONS       "${COMPILE_OPTS}"
                      LINK_OPTIONS          "${LINK_OPTS}")
