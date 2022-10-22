cmake_minimum_required(VERSION 3.16...3.21)

# Change obs-plugintemplate to your plugin's name in a machine-readable format (e.g.:
# obs-myawesomeplugin) and set
project(tuna VERSION 1.8.5)
add_library(${CMAKE_PROJECT_NAME} MODULE)
add_subdirectory(deps)

string(TIMESTAMP TODAY "%Y.%m.%d %H:%M")
add_definitions(-DBUILD_TIME="${TODAY}")
add_definitions(-DTUNA_VERSION="${PROJECT_VERSION}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/external")
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})
include_directories(${LIBCURL_INCLUDE_DIRS})
include_directories(${TAGLIB_INCLUDE_DIRS})
include_directories(${LIBMPDCLIENT_INCLUDE_DIRS})
include_directories(${CPPHTTPLIB_INCLUDE_DIRS})
add_definitions(${LIBCURL_DEFINITIONS})

# Replace `Your Name Here` with the name (yours or your organization's) you want to see as the
# author of the plugin (in the plugin's metadata itself and in the installers)
set(PLUGIN_AUTHOR "univrsal")

# Replace `com.example.obs-plugin-template` with a unique Bundle ID for macOS releases (used both in
# the installer and when submitting the installer for notarization)
set(MACOS_BUNDLEID "xyz.vrsal.${CMAKE_PROJECT_NAME}")

# Replace `me@contoso.com` with the maintainer email address you want to put in Linux packages
set(LINUX_MAINTAINER_EMAIL "uni@vrsal.xyz")

option(LOCAL_INSTALLATION "Whether to install for the current user (default: OFF)" OFF)

if (NOT CREDS)
    set(CREDS "MISSING")
    message(WARNING "Missing Spotify API info")
endif()
if (NOT LASTFM_CREDS)
    set(LASTFM_CREDS "MISSING")
    message(WARNING "Missing Last.fm API info")
endif()

mark_as_advanced(LASTFM_CREDS)
mark_as_advanced(CREDS)
add_definitions(-DSPOTIFY_CREDENTIALS=\"${CREDS}\")
add_definitions(-DLASTFM_CREDENTIALS=\"${LASTFM_CREDS}\")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  ./src/gui/tuna.qrc
  ./src/gui/tuna_gui.ui
  ./src/gui/output_edit_dialog.ui
  ./src/gui/music_control.ui
  ./src/gui/widgets/lastfm.ui
  ./src/gui/widgets/mpd.ui
  ./src/gui/widgets/window_title.ui
  ./src/gui/widgets/spotify.ui
  ./src/gui/widgets/icecast.ui
  ./src/gui/widgets/vlc.ui
  ./src/tuna_plugin.cpp
  ./src/util/constants.hpp
  ./src/util/config.cpp
  ./src/util/config.hpp
  ./src/util/creds.hpp
  ./src/gui/tuna_gui.cpp
  ./src/gui/tuna_gui.hpp
  ./src/gui/output_edit_dialog.cpp
  ./src/gui/output_edit_dialog.hpp
  ./src/gui/music_control.cpp
  ./src/gui/music_control.hpp
  ./src/gui/scrolltext.cpp
  ./src/gui/scrolltext.hpp
  ./src/query/music_source.hpp
  ./src/query/music_source.cpp
  ./src/query/spotify_source.cpp
  ./src/query/spotify_source.hpp
  ./src/query/mpd_source.cpp
  ./src/query/mpd_source.hpp
  ./src/query/window_source.cpp
  ./src/query/window_source.hpp
  ./src/query/lastfm_source.cpp
  ./src/query/lastfm_source.hpp
  ./src/query/gpmdp_source.cpp
  ./src/query/gpmdp_source.hpp
  ./src/query/web_source.cpp
  ./src/query/web_source.hpp
  ./src/query/icecast_source.cpp
  ./src/query/icecast_source.hpp
  ./src/query/song.cpp
  ./src/query/song.hpp
  ./src/util/format.cpp
  ./src/util/format.hpp
  ./src/source/progress.cpp
  ./src/source/progress.hpp
  ./src/util/cover_tag_handler.cpp
  ./src/util/cover_tag_handler.hpp
  ./src/query/vlc_obs_source.cpp
  ./src/query/vlc_obs_source.hpp
  ./src/util/tuna_thread.cpp
  ./src/util/tuna_thread.hpp
  ./src/util/utility.cpp
  ./src/util/utility.hpp
  ./src/util/web_server.cpp
  ./src/util/web_server.hpp
  ./src/util/window/window_helper.hpp
  ./src/gui/widgets/lastfm.cpp
  ./src/gui/widgets/lastfm.hpp
  ./src/gui/widgets/mpd.cpp
  ./src/gui/widgets/mpd.hpp
  ./src/gui/widgets/window_title.cpp
  ./src/gui/widgets/window_title.hpp
  ./src/gui/widgets/icecast.cpp
  ./src/gui/widgets/icecast.hpp
  ./src/gui/widgets/spotify.cpp
  ./src/gui/widgets/spotify.hpp
  ./src/gui/widgets/vlc.cpp
  ./src/gui/widgets/vlc.hpp
)

# Import libobs as main plugin dependency
include(cmake/ObsPluginHelpers.cmake)

if (UNIX AND NOT APPLE)
    option(WITH_DBUS  "Whether to add mpris support via dbus (Default: ON)" ON)

    if (WITH_DBUS)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULEPATH} ${CMAKE_SOURCE_DIR}/cmake)
        find_package(DBus REQUIRED)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${DBUS_LIBRARIES})
        target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR})
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE -DWITH_DBUS=1)
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE
          ./src/query/mpris_source.cpp
          ./src/query/mpris_source.hpp
          ./src/gui/widgets/mpris.cpp
          ./src/gui/widgets/mpris.hpp
          ./src/gui/widgets/mpris.ui
        )
    else()
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE -DWITH_DBUS=0)
    endif()
elseif(APPLE)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE -DWITH_DBUS=0)
elseif(WIN32)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE -DWITH_DBUS=0)
endif()
# Uncomment these lines if you want to use the OBS Frontend API in your plugin

if (LOCAL_INSTALLATION)

    if (UNIX AND NOT APPLE)
        # Installs into home directory
        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            # Copy to obs-studio dev environment for immediate testing
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/bin/64bit"
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/data"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/bin/64bit"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${PROJECT_SOURCE_DIR}/data"
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/data"
        )
    elseif (WIN32)
        # Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when building with VS2019)
        if (MSVC)
            add_definitions(/MP /d2FH4-)
        endif()

        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            # Copy to obs-studio dev environment for immediate testing
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E copy
                    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                    "${libobs_DIR}/../rundir/$<CONFIG>/obs-plugins/64bit")
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E copy
                    "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                    "${libobs_DIR}/../rundir/$<CONFIG>/obs-plugins/64bit")
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E make_directory
                    "${libobs_DIR}/../rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}")
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E copy_directory
                    "${PROJECT_SOURCE_DIR}/data"
                    "${libobs_DIR}/../rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}")
        )
    endif()
endif()

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if (WIN32)
    target_sources(${CMAKE_PROJECT_NAME} PUBLIC "./src/util/window/window_helper_win.cpp")
elseif(UNIX AND NOT APPLE)
    target_sources(${CMAKE_PROJECT_NAME} PUBLIC "./src/util/window/window_helper_nix.cpp")
elseif(APPLE)
    target_sources(${CMAKE_PROJECT_NAME} PUBLIC "./src/util/window/window_helper_mac.mm")
endif()

# Uncomment those lines if you want to use Qt in your plugin
find_qt(COMPONENTS Widgets Core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt::Core Qt::Widgets tag mpdclient ZLIB::ZLIB CURL::libcurl)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

configure_file(src/plugin-macros.h.in ${CMAKE_SOURCE_DIR}/src/plugin-macros.generated.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-macros.generated.h)

# /!\ TAKE NOTE: No need to edit things past this point /!\

# --- Platform-independent build settings ---

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)


# --- End of section ---

# --- Windows-specific build settings and tasks ---
if(OS_WINDOWS)
  configure_file(cmake/bundle/windows/installer-Windows.iss.in
                 ${CMAKE_BINARY_DIR}/installer-Windows.generated.iss)

  if(MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4)
  endif()
  # --- End of section ---

  # -- macOS specific build settings and tasks --
elseif(OS_MACOS)
  configure_file(cmake/bundle/macos/installer-macos.pkgproj.in
                 ${CMAKE_BINARY_DIR}/installer-macos.generated.pkgproj)

  set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
  set(MACOSX_PLUGIN_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}")
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")

  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
  # --- End of section ---

  # --- Linux-specific build settings and tasks ---
else()
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
endif()
# --- End of section ---

setup_plugin_target(${CMAKE_PROJECT_NAME})
