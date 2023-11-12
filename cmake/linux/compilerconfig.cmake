# CMake Linux compiler configuration module

include_guard(GLOBAL)

include(ccache)
include(compiler_common)

option(ENABLE_COMPILER_TRACE "Enable Clang time-trace (required Clang and Ninja)" OFF)
mark_as_advanced(ENABLE_COMPILER_TRACE)

add_compile_options(
  -fopenmp-simd)

# Add support for color diagnostics and CMake switch for warnings as errors to CMake < 3.24
if(CMAKE_VERSION VERSION_LESS 3.24.0)
  add_compile_options($<$<C_COMPILER_ID:Clang>:-fcolor-diagnostics> $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>)

else()
  set(CMAKE_COLOR_DIAGNOSTICS ON)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  # Disable false-positive warning in GCC 12.1.0 and later
  add_compile_options(-Wno-error=maybe-uninitialized)

  # Add warning for infinite recursion (added in GCC 12)
  if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0.0)
    add_compile_options(-Winfinite-recursion)
  endif()
endif()

# Enable compiler and build tracing (requires Ninja generator)
if(ENABLE_COMPILER_TRACE AND CMAKE_GENERATOR STREQUAL "Ninja")
  add_compile_options($<$<COMPILE_LANG_AND_ID:C,Clang>:-ftime-trace> $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-ftime-trace>)
else()
  set(ENABLE_COMPILER_TRACE
      OFF
      CACHE STRING "Enable Clang time-trace (required Clang and Ninja)" FORCE)
endif()

add_compile_definitions($<$<CONFIG:DEBUG>:DEBUG> $<$<CONFIG:DEBUG>:_DEBUG> SIMDE_ENABLE_OPENMP)
