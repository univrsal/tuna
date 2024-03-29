# CMake macOS compiler configuration module

include_guard(GLOBAL)

include(ccache)
include(compiler_common)

add_compile_options(-fopenmp-simd)

if(XCODE)
  # Use Xcode's standard architecture selection
  set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
  # Enable dSYM generation for Release builds
  string(APPEND CMAKE_C_FLAGS_RELEASE " -g")
  string(APPEND CMAKE_CXX_FLAGS_RELEASE " -g")
else()
  option(ENABLE_COMPILER_TRACE "Enable clang time-trace (requires Ninja)" OFF)
  mark_as_advanced(ENABLE_COMPILER_TRACE)
  compile_options(-Wno-shorten-64-to-32 -Wno-unused-parameter)
  # Enable stripping of dead symbols when not building for Debug configuration
  set(_release_configs RelWithDebInfo Release MinSizeRel)
  if(CMAKE_BUILD_TYPE IN_LIST _release_configs)
    add_link_options(LINKER:-dead_strip)
  endif()

  # Enable color diagnostics for AppleClang
  set(CMAKE_COLOR_DIAGNOSTICS ON)
  # Set universal architectures via CMake flag for non-Xcode generators
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

  # Enable compiler and build tracing (requires Ninja generator)
  if(ENABLE_COMPILER_TRACE AND CMAKE_GENERATOR STREQUAL "Ninja")
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-ftime-trace> $<$<COMPILE_LANGUAGE:CXX>:-ftime-trace>)
  else()
    set(ENABLE_COMPILER_TRACE
        OFF
        CACHE STRING "Enable clang time-trace (requires Ninja)" FORCE)
  endif()
endif()

add_compile_definitions($<$<CONFIG:DEBUG>:DEBUG> $<$<CONFIG:DEBUG>:_DEBUG> SIMDE_ENABLE_OPENMP)
