cmake_minimum_required(VERSION 3.5)
project(tuna)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Qt5Widgets REQUIRED)
find_package(Libcurl REQUIRED)
include_directories(${LIBCURL_INCLUDE_DIRS})
add_definitions(${LIBCURL_DEFINITIONS})

if (NOT Qt5Widgets_FOUND)
    message(FATAL_ERROR "Failed to find Qt5")
endif ()

# Read Spotify credentials from creds.txt
# In this format {client_id}:{client_secre}
# Make sure that there's no empty new line
file(READ "creds.txt" CREDS)
string(REGEX REPLACE "\n$" "" CREDS "${CREDS}")
add_definitions(-DSPOTIFY_CREDENTIALS=\"${CREDS}\")

set(tuna_ui
        src/gui/tuna_gui.ui
        src/gui/output_edit_dialog.ui)
find_package(LibVLC QUIET)


if (UNIX AND NOT APPLE)
    add_definitions(-DLINUX=1)
        find_package(Taglib)
    set(tuna_platform_sources "./src/util/window/window_helper_nix.cpp")
        set(tuna_platform_deps
            mpdclient
            tag)
        set(tuna_platform_includes
            ${TAGLIB_INCLUDES})
endif()

if (APPLE)
    add_definitions(-DMACOS=1)
    set(tuna_platform_sources "./src/util/window/window_helper_mac.mm")
        find_library(COCOA Cocoa)
        find_Library(MPDCLIENT mpdclient)
        find_library(TAG tag)
        find_path(MPDCLIENT_INCLUDES mpd/client.h)
        find_path(TAG_INCLUDES taglib/tag.h)

    include_directories(${COCOA})
        include_directories(${TAG_INCLUDES})
        include_directories(${MPDCLIENT_INCLUDES})
    set_source_files_properties("./src/util/window/window_helper_mac.mm"
        PROPERTIES COMPILE_FLAGS "-fobjc-arc")

        set(tuna_platform_deps
                ${COCOA}
                ${MPDCLIENT}
                ${TAG})
endif()

if (UNIX)
    add_definitions(-DUNIX=1)

    set(tuna_platform_sources ${tuna_platform_sources}
        "./src/util/cover_tag_handler.cpp"
        "./src/util/cover_tag_handler.hpp")
endif ()

if (MSVC)
   set(tuna_platform_sources "./src/util/window/window_helper_win.cpp")
   set(tuna_platform_deps
            w32-pthreads)
endif ()

if (NOT LIBVLC_INCLUDES_FOUND AND ENABLE_VLC)
    message(STATUS "[tuna] LibVLC includes not found but set as enabled")
    add_definitions(-DDISABLE_TUNA_VLC=1)
elseif (NOT LIBVLC_INCLUDES_FOUND)
    message(STATUS "[tuna] LibVLC includes not found, VLC source support disabled")
    message(STATUS "[tuna] If you want VLC support, clone the VLC repo and point VLC_INCLUDE_DIR to <repo>/include/vlc ")
    add_definitions(-DDISABLE_TUNA_VLC=1)
else()
    message(STATUS "[tuna] LibVLC headers found, VLC source support enabled")
endif()

if (DISABLE_VLC)
    message(STATUS "VLC integration disabled")
    add_definitions(-DDISABLE_TUNA_VLC=1)
else()
    include_directories(${LIBVLC_INCLUDE_DIRS})
    add_definitions(${LIBVLC_DEFINITIONS})
    set(tuna_vlc_source
        ./src/query/vlc_obs_source.cpp
        ./src/util/vlc_internal.h
        ./src/util/vlc_internal.c)
endif()

set(tuna_sources
    ./src/tuna_plugin.cpp
    ./src/util/constants.hpp
    ./src/util/config.cpp
    ./src/util/config.hpp
    ./src/util/creds.hpp
    ./src/gui/tuna_gui.cpp
    ./src/gui/tuna_gui.hpp
    ./src/gui/output_edit_dialog.cpp
    ./src/gui/output_edit_dialog.hpp
    ./src/query/music_source.hpp
    ./src/query/music_source.cpp
    ./src/query/spotify_source.cpp
    ./src/query/spotify_source.hpp
    ./src/query/mpd_source.cpp
    ./src/query/mpd_source.hpp
    ./src/query/window_source.cpp
    ./src/query/window_source.hpp
    ./src/query/song.cpp
    ./src/query/song.hpp
    ./src/util/format.cpp
    ./src/util/format.hpp
    ./src/source/progress.cpp
    ./src/source/progress.hpp
    ${tuna_vlc_source}
    ./src/query/vlc_obs_source.hpp
    ./src/util/tuna_thread.cpp
    ./src/util/tuna_thread.hpp
    ./src/util/utility.cpp
    ./src/util/utility.hpp
    ./src/util/window/window_helper.hpp)

add_library(tuna MODULE
    ${tuna_sources}
    ${tuna_ui}
    ${tuna_platform_sources})

include_directories(
        SYSTEM "${CMAKE_SOURCE_DIR}/libobs"
        "../../UI/obs-frontend-api"
        ${tuna_platform_includes}
        ${Qt5Core_INCLUDES}
        ${Qt5Widgets_INCLUDES})

target_link_libraries(tuna
    libobs
    jansson
    Qt5::Widgets
    Qt5::Core
    obs-frontend-api
    ${LIBCURL_LIBRARIES}
    ${tuna_platform_deps})

set_property(TARGET tuna PROPERTY CXX_STANDARD 14)

install_obs_plugin_with_data(tuna data)
