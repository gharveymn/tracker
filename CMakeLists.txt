CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
PROJECT(tracker)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_EXTENSIONS OFF)

INCLUDE_DIRECTORIES(external/plf_list)
INCLUDE_DIRECTORIES(external/optional_ref/include)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-report")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

if (UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
endif()

ADD_EXECUTABLE(tracker main.cpp tracker.h)
