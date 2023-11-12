cmake_minimum_required(VERSION 3.16...3.26)


include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(LOCAL_INSTALLATION "Copy to ~/.config/obs-studio/plugins after build" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

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

if (NOT CREDS)
    set(CREDS "MISSING")
    message(WARNING "Missing Spotify API info")
endif()
if (NOT LASTFM_CREDS)
    set(LASTFM_CREDS "MISSING")
    message(WARNING "Missing Last.fm API info")
endif()

# Get the current working branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get the latest commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "-DGIT_BRANCH=\"${GIT_BRANCH}\"")
target_compile_definitions(${PROJECT_NAME} PRIVATE
"-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")

mark_as_advanced(LASTFM_CREDS)
mark_as_advanced(CREDS)
add_definitions(-DSPOTIFY_CREDENTIALS=\"${CREDS}\")
add_definitions(-DLASTFM_CREDENTIALS=\"${LASTFM_CREDS}\")
add_subdirectory(src)

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
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE
          ./src/query/wmc_source.cpp
          ./src/query/wmc_source.hpp
          ./src/gui/widgets/wmc.cpp
          ./src/gui/widgets/wmc.hpp
          ./src/gui/widgets/wmc.ui
        )
endif()

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


find_qt(COMPONENTS Widgets Core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt::Core Qt::Widgets tag mpdclient ZLIB::ZLIB CURL::libcurl)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

configure_file(src/plugin-macros.h.in ${CMAKE_SOURCE_DIR}/src/plugin-macros.generated.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-macros.generated.h)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

