set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

# --- On Linux: use system libraries (found via pkg-config in root CMakeLists) ---
# Nothing else to do here for taglib/libmpdclient.

# --- On Windows / macOS: build taglib from source, libmpdclient via FetchContent ---
if(WIN32 OR APPLE)
  include(FetchContent)

  # taglib: build from source for correct include layout (taglib/ape/apefile.h etc.)
  FetchContent_Declare(
    taglib
    GIT_REPOSITORY https://github.com/taglib/taglib.git
    GIT_TAG v2.3.1
  )
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(ENABLE_CCACHE OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(taglib)

  target_include_directories(tag PUBLIC
    "$<BUILD_INTERFACE:${taglib_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${taglib_BINARY_DIR}>"
  )
  set(TAGLIB_INCLUDE_DIRS
    ${taglib_SOURCE_DIR}
    ${taglib_BINARY_DIR}
    CACHE STRING "" FORCE
  )
  set(TAGLIB_TARGET tag CACHE STRING "" FORCE)

  # ---- libmpdclient (v2.22, uses Meson so we compile sources manually) ----
  FetchContent_Declare(
    libmpdclient
    GIT_REPOSITORY https://github.com/MusicPlayerDaemon/libmpdclient.git
    GIT_TAG v2.22
  )

  FetchContent_GetProperties(libmpdclient)
  if(NOT libmpdclient_POPULATED)
    FetchContent_Populate(libmpdclient)
  endif()

  # Generate config.h
  set(MAJOR_VERSION 2)
  set(MINOR_VERSION 22)
  set(PATCH_VERSION 0)
  set(MPD_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")

  set(MPD_DEFAULT_HOST "localhost")
  set(MPD_DEFAULT_PORT 6600)

  if(WIN32)
    set(MPD_DEFAULT_SOCKET "")
  else()
    set(MPD_DEFAULT_SOCKET "/var/run/mpd/socket")
  endif()

  include(CheckFunctionExists)
  check_function_exists(strndup HAVE_STRNDUP)
  check_function_exists(getaddrinfo HAVE_GETADDRINFO)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpd.config.h.in
    ${libmpdclient_BINARY_DIR}/config.h
  )
  configure_file(
    ${libmpdclient_SOURCE_DIR}/include/mpd/version.h.in
    ${libmpdclient_BINARY_DIR}/include/mpd/version.h
    @ONLY
  )

  file(GLOB libmpdclient_SRC "${libmpdclient_SOURCE_DIR}/src/*.c")

  add_library(mpdclient STATIC ${libmpdclient_SRC})
  target_include_directories(mpdclient PUBLIC
    ${libmpdclient_SOURCE_DIR}/include
    ${libmpdclient_BINARY_DIR}
    ${libmpdclient_BINARY_DIR}/include
  )
  target_compile_definitions(mpdclient PRIVATE HAVE_CONFIG_H)
  set_target_properties(mpdclient PROPERTIES LINKER_LANGUAGE C)
  set_property(TARGET mpdclient PROPERTY POSITION_INDEPENDENT_CODE ON)

  if(APPLE)
    target_compile_options(mpdclient PUBLIC $<$<COMPILE_LANGUAGE:C>:-Wno-everything>)
  elseif(NOT APPLE)
    target_compile_options(mpdclient PUBLIC -w)
  endif()

  set(LIBMPDCLIENT_INCLUDE_DIRS
    ${libmpdclient_SOURCE_DIR}/include
    ${libmpdclient_BINARY_DIR}/include
    CACHE STRING "" FORCE
  )
  set(LIBMPDCLIENT_TARGET mpdclient CACHE STRING "" FORCE)

endif()
