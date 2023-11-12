# CMake common compiler options module

include_guard(GLOBAL)

# Set C and C++ language standards to C17 and C++17
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
  set(CMAKE_C_STANDARD 17)
else()
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Set symbols to be hidden by default for C and C++
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
