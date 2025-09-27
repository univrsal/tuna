set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

add_subdirectory(taglib)
set_property(TARGET tag PROPERTY POSITION_INDEPENDENT_CODE ON)

# Define paths to your scripts
set(LINUX_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/deps/build-libmpdclient.sh")
set(WINDOWS_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/deps/build-libmpdclient.ps1")


if(WIN32)
    add_custom_target(build_libbar ALL
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.lib
        COMMAND powershell.exe -ExecutionPolicy Bypass -File  "${WINDOWS_BUILD_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps
        COMMENT "Building libmdclient.lib"
    )

    set(LIBMPDCLIENT_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.lib CACHE STRING "" FORCE)
else()

    add_custom_target(build_libmpdclient ALL
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.a
        COMMAND bash "${LINUX_BUILD_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps
        COMMENT "Building libmpdclient.a"
    )
    set(LIBMPDCLIENT_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/lib/libmpdclient.a CACHE STRING "" FORCE)
endif()


set(LIBMPDCLIENT_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/install/include CACHE STRING "" FORCE)

