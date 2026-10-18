# FindSDL2_ttf.cmake - Emscripten SDL2_ttf Support

if(EMSCRIPTEN)
    message(STATUS "Using Emscripten's built-in SDL2_ttf (via -sUSE_SDL_TTF=2)")

    if(NOT TARGET SDL2_ttf::SDL2_ttf)
        add_library(SDL2_ttf::SDL2_ttf INTERFACE IMPORTED GLOBAL)
    endif()

    set(SDL2_ttf_FOUND TRUE)
    set(SDL2_ttf_DIR "emscripten-builtin")
    set(SDL2_ttf_INCLUDE_DIRS "")
    set(SDL2_ttf_LIBRARIES SDL2_ttf::SDL2_ttf)
    set(SDL2_ttf_VERSION "2.0.0")

    mark_as_advanced(SDL2_ttf_DIR SDL2_ttf_INCLUDE_DIRS SDL2_ttf_LIBRARIES)

    message(STATUS "SDL2_ttf_FOUND = ${SDL2_ttf_FOUND}")
else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SDL2_ttf QUIET SDL2_ttf)
    endif()

    if(NOT SDL2_ttf_FOUND)
        message(WARNING "SDL2_ttf not found. The TrueType font will be unavailable.")
    endif()
endif()
