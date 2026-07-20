set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

# --- On Linux / macOS: use system libraries (found via pkg-config in root CMakeLists) ---
# Nothing else to do here for taglib/libmpdclient.

# --- On Windows: get taglib from vcpkg, build libmpdclient via FetchContent ---
if(WIN32)
  include(FetchContent)

  # Use vcpkg to provide taglib (taglibConfig.cmake).
  find_package(taglib CONFIG REQUIRED)

  # vcpkg's taglib target is TagLib::tag (or TagLib::tag_c if using its C API).
  # Use whichever target was imported — we just need the include dirs.
  if(TARGET TagLib::tag)
    set(TAGLIB_TARGET TagLib::tag CACHE STRING "" FORCE)
    get_target_property(_tag_iface_include TagLib::tag INTERFACE_INCLUDE_DIRECTORIES)
  elseif(TARGET TagLib::tag_c)
    set(TAGLIB_TARGET TagLib::tag_c CACHE STRING "" FORCE)
    get_target_property(_tag_iface_include TagLib::tag_c INTERFACE_INCLUDE_DIRECTORIES)
  else()
    message(FATAL_ERROR "vcpkg taglib target not found (expected TagLib::tag or TagLib::tag_c)")
  endif()
  set(TAGLIB_INCLUDE_DIRS ${_tag_iface_include} CACHE STRING "" FORCE)

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
