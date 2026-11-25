cmake_minimum_required(VERSION 3.12)
project(HexStartup VERSION 2.0 LANGUAGES C CXX)

# HexStartup++: converts 16 color images (PPM / PNG) into Hexen STARTUP, NOTCH and NETNOTCH lumps.
#
#   git submodule update --init
#   cmake -S . -B build
#   cmake --build build
#
# MT2D options are forwarded to the engine, e.g. -DMT2D_SDL=ON for an SDL2 window instead of the terminal.

# --------------------------------------------------------------------------------------------
# MT2D engine (git submodule, pinned to a release tag)
# --------------------------------------------------------------------------------------------
set(MT2D_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/MT2D")
if(NOT EXISTS "${MT2D_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "MT2D was not found in external/MT2D. Fetch the submodule with:\n"
        "  git submodule update --init")
endif()
add_subdirectory("${MT2D_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/MT2D" EXCLUDE_FROM_ALL)

# --------------------------------------------------------------------------------------------
# libpng (+ zlib)
# The system package is used when available; MSVC can fall back to the prebuilt libs in Libs/.
# --------------------------------------------------------------------------------------------
find_package(PNG QUIET)
if(PNG_FOUND)
    set(HEXSTARTUP_PNG_LIBS PNG::PNG)
elseif(MSVC)
    set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libs")
    add_library(hexstartup_zlib STATIC IMPORTED)
    set_target_properties(hexstartup_zlib PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG   "${LIBS_DIR}/Zlib/Lib/zlibstatD.lib"
        IMPORTED_LOCATION_RELEASE "${LIBS_DIR}/Zlib/Lib/zlibstatR.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/Zlib/Include")
    add_library(hexstartup_png STATIC IMPORTED)
    set_target_properties(hexstartup_png PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG   "${LIBS_DIR}/LibPNG/Lib/libpngd.lib"
        IMPORTED_LOCATION_RELEASE "${LIBS_DIR}/LibPNG/Lib/libpngr.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/LibPNG/Include"
        INTERFACE_LINK_LIBRARIES hexstartup_zlib)
    set(HEXSTARTUP_PNG_LIBS hexstartup_png)
    message(STATUS "HexStartup: using the bundled libpng/zlib from Libs/")
else()
    message(FATAL_ERROR "libpng was not found (install libpng-dev / libpng-devel)")
endif()

# --------------------------------------------------------------------------------------------
# HexStartup
# --------------------------------------------------------------------------------------------
add_executable(HexStartup
    Source.cpp
    Image.cpp
    Hexen.cpp
    PPM.cpp
    _PNG.cpp
)
target_link_libraries(HexStartup PRIVATE mt2d::mt2d ${HEXSTARTUP_PNG_LIBS})
set_target_properties(HexStartup PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
if(MSVC)
    target_compile_definitions(HexStartup PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    # The MT2D API takes char* and the code passes string literals to it.
    target_compile_options(HexStartup PRIVATE -Wno-write-strings)
endif()
