set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

# --- taglib: use FetchContent everywhere for consistent v2.x layout ---
include(FetchContent)
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

set(TAGLIB_INCLUDE_DIRS
  ${taglib_SOURCE_DIR}
  ${taglib_SOURCE_DIR}/taglib
  ${taglib_SOURCE_DIR}/taglib/toolkit
  ${taglib_SOURCE_DIR}/taglib/asf
  ${taglib_SOURCE_DIR}/taglib/mpeg
  ${taglib_SOURCE_DIR}/taglib/mpeg/id3v1
  ${taglib_SOURCE_DIR}/taglib/mpeg/id3v2
  ${taglib_SOURCE_DIR}/taglib/mpeg/id3v2/frames
  ${taglib_SOURCE_DIR}/taglib/ogg
  ${taglib_SOURCE_DIR}/taglib/ogg/flac
  ${taglib_SOURCE_DIR}/taglib/flac
  ${taglib_SOURCE_DIR}/taglib/mpc
  ${taglib_SOURCE_DIR}/taglib/mp4
  ${taglib_SOURCE_DIR}/taglib/ogg/vorbis
  ${taglib_SOURCE_DIR}/taglib/ogg/speex
  ${taglib_SOURCE_DIR}/taglib/ogg/opus
  ${taglib_SOURCE_DIR}/taglib/ape
  ${taglib_SOURCE_DIR}/taglib/wavpack
  ${taglib_SOURCE_DIR}/taglib/trueaudio
  ${taglib_SOURCE_DIR}/taglib/riff
  ${taglib_SOURCE_DIR}/taglib/riff/aiff
  ${taglib_SOURCE_DIR}/taglib/riff/wav
  ${taglib_SOURCE_DIR}/taglib/mod
  ${taglib_SOURCE_DIR}/taglib/s3m
  ${taglib_SOURCE_DIR}/taglib/it
  ${taglib_SOURCE_DIR}/taglib/xm
  ${taglib_SOURCE_DIR}/taglib/dsf
  ${taglib_SOURCE_DIR}/taglib/dsdiff
  ${taglib_SOURCE_DIR}/taglib/shorten
  ${taglib_SOURCE_DIR}/taglib/matroska
  ${taglib_SOURCE_DIR}/taglib/matroska/ebml
  ${taglib_BINARY_DIR}
  CACHE STRING "" FORCE
)
set(TAGLIB_TARGET tag CACHE STRING "" FORCE)

# --- libmpdclient: system on Linux, FetchContent on macOS/Windows ---
if(WIN32 OR APPLE)
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
