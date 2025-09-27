set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

add_subdirectory(taglib)
set_property(TARGET tag PROPERTY POSITION_INDEPENDENT_CODE ON)


include(ExternalProject)

ExternalProject_Add(libmdclient
    PREFIX ${CMAKE_BINARY_DIR}/libmdclient
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient
    BINARY_DIR ${CMAKE_BINARY_DIR}/libmdclient/build
    INSTALL_DIR ${CMAKE_BINARY_DIR}/libmdclient/install
    CONFIGURE_COMMAND meson setup <BINARY_DIR> <SOURCE_DIR> --prefix=<INSTALL_DIR>
    BUILD_COMMAND meson compile -C <BINARY_DIR>
    INSTALL_COMMAND meson install -C <BINARY_DIR>
)

