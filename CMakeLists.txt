cmake_minimum_required(VERSION 3.5)
project(tuna)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Widgets REQUIRED)
if (NOT Qt5Widgets_FOUND)
    message(FATAL_ERROR "Failed to find Qt5")
endif ()

set(tuna_ui
        src/gui/tuna_gui.ui)

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    add_definitions(-DLINUX=1)
    add_definitions(-DUNIX=1)

    find_path(MPD_INCLUDE_DIRS mpd.h)
    set(tuna_platform_deps
            mpdclient)
    set(tuna_plaform_sources "")
endif ()

if (MSVC)
   set(tuna_platform_sources "")
   set(tuna_platform_deps
            w32-pthreads)
endif ()

set(tuna_sources
    ./src/tuna_plugin.cpp
    ./src/util/constants.hpp
    ./src/util/config.cpp
    ./src/util/config.hpp
    ./src/util/creds.hpp
    ./src/gui/tuna_gui.cpp
    ./src/gui/tuna_gui.hpp
    ./src/query/music_source.hpp
    ./src/query/spotify_source.cpp
    ./src/query/spotify_source.hpp
    ./src/query/mpd_source.cpp
    ./src/query/mpd_source.hpp
    ./src/util/tuna_thread.cpp
    ./src/util/tuna_thread.hpp
    ./src/util/utility.cpp
    ./src/util/utility.hpp)

add_library(tuna MODULE
    ${tuna_sources}
    ${tuna_ui}
    ${tuna_platform_sources})

include_directories(
        SYSTEM "${CMAKE_SOURCE_DIR}/libobs"
        "../../UI/obs-frontend-api"
        ${Qt5Core_INCLUDES}
        ${Qt5Widgets_INCLUDES})

target_link_libraries(tuna
    libobs
    jansson
    curl
    Qt5::Widgets
    obs-frontend-api
    ${tuna_platform_deps})

install_obs_plugin_with_data(tuna data)
