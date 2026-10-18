if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(FreeSerf VERSION 0.5.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# ============================================================================
# EMSCRIPTEN / WEBASSEMBLY SUPPORT
# ============================================================================
if(EMSCRIPTEN)
    # Stelle sicher dass unsere FindSDL2.cmake Module zuerst gefunden werden
    list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
endif()

include(GitTools)
git_make_version(FREESERF_VERSION ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_VERSION})
message(STATUS "FREESERF_VERSION = ${FREESERF_VERSION}")

set(CMAKE_CXX_STANDARD 11)

include_directories(${INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(VCS_TAG "${FREESERF_VERSION}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version-vcs.h.in
               "${CMAKE_CURRENT_BINARY_DIR}/src/version-vcs.h"
               @ONLY)

add_definitions(-DPACKAGE_BUGREPORT="https://github.com/freeserf/freeserf/issues")

# ============================================================================
# EMSCRIPTEN / WEBASSEMBLY BUILD FLAGS
# ============================================================================
if(EMSCRIPTEN)
    # KEIN .html Suffix! Wir verwenden unsere eigene index.html
    # set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    # SDL2 Ã¼ber Emscripten Ports
    # (Die Targets werden von cmake/FindSDL2.cmake erstellt)
    # Muss auch beim Kompilieren aktiv sein, damit die SDL-Header gefunden werden.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL=2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL_MIXER=2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL_IMAGE=2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL_TTF=2")
    # Nur WAV/MIDI/MOD - kein OGG (vermeidet den ogg-Port-Download)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sSDL2_MIXER_FORMATS=mod,mid")

    # C++ Exception Support
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

    # Optimierungen
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto")
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -gsource-map -sASSERTIONS=1")
    endif()

    # Linker-Flags (Emscripten)
    # MODULARIZE: FreeSerf.js exportiert eine Factory-Funktion (FreeSerf(Module)),
    # die von web/index.html verwendet wird.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sMODULARIZE=1 -sEXPORT_NAME=FreeSerf")

    # Memory Settings
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sINITIAL_MEMORY=67108864")   # 64MB
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sMAXIMUM_MEMORY=2147483648") # 2GB

    # Browser Environment
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sENVIRONMENT=web")

    # Export FS fÃ¼r Laufzeit-Dateiladen (Spieldaten)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,FS,callMain")

    # Filesystem Support (fÃ¼r Savegames via IndexedDB)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lidbfs.js")

    message(STATUS "Game data will be loaded at runtime via FS")
    message(STATUS "Emscripten CXX flags: ${CMAKE_CXX_FLAGS}")
    message(STATUS "Emscripten LINK flags: ${CMAKE_EXE_LINKER_FLAGS}")
    message(STATUS "==============================================")
endif()
# ============================================================================

include(CppLint)
include(Translations)
enable_check_style()

add_subdirectory(src)

option(ENABLE_TESTS "Enable compilation of tests" ON)
if(ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
