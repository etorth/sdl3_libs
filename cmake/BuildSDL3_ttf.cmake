include(ExternalProject)

ExternalProject_Add(
    libSDL3_ttf

    DEPENDS libSDL3

    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
    GIT_SHALLOW TRUE

    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DBUILD_SHARED_LIBS=OFF
        -DSDL3TTF_STATIC=ON
        -DSDL3TTF_SHARED=OFF

    BUILD_BYPRODUCTS
        <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL3_ttf${CMAKE_STATIC_LIBRARY_SUFFIX}
)

ExternalProject_Get_Property(libSDL3_ttf INSTALL_DIR)

add_library(SDL3_ttf::SDL3_ttf STATIC IMPORTED GLOBAL)
set_target_properties(SDL3_ttf::SDL3_ttf PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL3_ttf${CMAKE_STATIC_LIBRARY_SUFFIX}
    INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/include
)
add_dependencies(SDL3_ttf::SDL3_ttf libSDL3_ttf)
