add_subdirectory(taglib)
set_property(TARGET tag PROPERTY POSITION_INDEPENDENT_CODE 1)
set(LIBMPDCLIENT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include")
set(LIBMPDCLIENT_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include CACHE STRING "" FORCE)
set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

set(MAJOR_VERSION 2)
set(MINOR_VERSION 22)
set(PATCH_VERSION 0)
set(MPD_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
set(MPD_DEFAULT_PORT 6600)
set(MPD_DEFAULT_HOST "localhost")
set(MPD_DEFAULT_SOCKET "/var/run/mpd/socket")

find_package(ZLIB REQUIRED)

include(CheckFunctionExists)
include(CTest)
include(FeatureSummary)
include(GNUInstallDirs)

check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(getaddrinfo HAVE_GETADDRINFO)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include)
include_directories(${ZLIB_INCLUDE_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include/mpd/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include/mpd/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libmpd.config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/src/config.h)

add_definitions(-DHAVE_CONFIG_H)


if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

endif()

file(GLOB libmpdclient_SRC "${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/src/*.c")
add_library(mpdclient STATIC ${libmpdclient_SRC})

set_property(TARGET mpdclient PROPERTY POSITION_INDEPENDENT_CODE 1)

set_target_properties(mpdclient PROPERTIES LINKER_LANGUAGE C)

if (APPLE)
target_compile_options(mpdclient
  PUBLIC $<$<COMPILE_LANGUAGE:C>:-Wno-everything>
)
elseif(UNIX AND NOT APPLE)
target_compile_options(mpdclient
  PUBLIC -w
)
endif()

