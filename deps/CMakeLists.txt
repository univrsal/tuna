set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

add_subdirectory(taglib)
set_property(TARGET tag PROPERTY POSITION_INDEPENDENT_CODE ON)

# Define paths to your scripts
set(MPD_LINUX_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/deps/build-libmpdclient.sh")
set(MPD_WINDOWS_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/deps/build-libmpdclient.ps1")
set(TL_LINUX_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/deps/build-taglib.sh")
set(TL_WINDOWS_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/deps/build-taglib.ps1")


if(WIN32)
    add_custom_target(build_libmpdclient ALL
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.lib
        COMMAND powershell.exe -ExecutionPolicy Bypass -File  "${MPD_WINDOWS_BUILD_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps
        COMMENT "Building libmdclient.lib"
    )
    add_custom_target(build_taglib ALL
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/taglib/install/lib/taglib.lib
        COMMAND powershell.exe -ExecutionPolicy Bypass -File  "${TL_WINDOWS_BUILD_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps
        COMMENT "Building taglib"
    )

    set(LIBMPDCLIENT_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.lib CACHE STRING "" FORCE)
    set(TAGLIB_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/taglib/install/lib/taglib.lib CACHE STRING "" FORCE)

else()
    add_custom_target(build_libmpdclient ALL
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.a
        COMMAND bash "${MPD_LINUX_BUILD_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps
        COMMENT "Building libmpdclient.a"
    )

    add_custom_target(build_taglib ALL
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/taglib/install/lib/libtag.a
        COMMAND bash "${TL_LINUX_BUILD_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps
        COMMENT "Building taglib"
    )

    set(LIBMPDCLIENT_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.a CACHE STRING "" FORCE)
    set(TAGLIB_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/taglib/install/lib/libtag.a CACHE STRING "" FORCE)
endif()


set(LIBMPDCLIENT_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/include CACHE STRING "" FORCE)
set(TAGLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/taglib/install/include CACHE STRING "" FORCE)
