SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ADD_EXECUTABLE(test_tracker main.cpp)
TARGET_LINK_LIBRARIES(test_tracker tracker)

IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  TARGET_COMPILE_OPTIONS(test_tracker PUBLIC -stdlib=libc++ -ftime-trace --coverage -fno-rtti -Wall -Wextra)
  TARGET_LINK_OPTIONS(test_tracker PUBLIC -Wl,--allow-multiple-definition --coverage)
  TARGET_LINK_LIBRARIES(test_tracker c++ c++abi)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  TARGET_COMPILE_OPTIONS(test_tracker PUBLIC -fno-rtti -Wall -Wextra)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  TARGET_COMPILE_OPTIONS(test_tracker PUBLIC /W4 /GR-)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")

ENDIF()

if (UNIX)
  TARGET_COMPILE_OPTIONS(test_tracker PUBLIC -fsanitize=address)
  TARGET_LINK_OPTIONS(test_tracker PUBLIC -fsanitize=address)
endif()

SET_TARGET_PROPERTIES(test_tracker PROPERTIES
                      CXX_STANDARD          11
                      CXX_STANDARD_REQUIRED NO
                      CXX_EXTENSIONS        NO)
