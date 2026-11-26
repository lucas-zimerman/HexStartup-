cmake_minimum_required(VERSION 3.15)
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
# The system package is used when available; otherwise both are built from the git submodules
# in external/zlib and external/libpng. -DHEXSTARTUP_BUNDLED_PNG=ON always uses the submodules.
# --------------------------------------------------------------------------------------------
option(HEXSTARTUP_BUNDLED_PNG "Build libpng and zlib from the git submodules instead of using the system ones" OFF)

if(NOT HEXSTARTUP_BUNDLED_PNG)
    find_package(PNG QUIET)
endif()

if(PNG_FOUND)
    set(HEXSTARTUP_PNG_LIBS PNG::PNG)
else()
    set(ZLIB_DIR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib")
    set(LIBPNG_DIR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/external/libpng")
    if(NOT EXISTS "${ZLIB_DIR_SRC}/CMakeLists.txt" OR NOT EXISTS "${LIBPNG_DIR_SRC}/CMakeLists.txt")
        message(FATAL_ERROR
            "libpng was not found on the system and the zlib/libpng submodules are missing.\n"
            "Install libpng (e.g. libpng-dev) or fetch the submodules with:\n"
            "  git submodule update --init")
    endif()

    # zlib: static library only, no tests or install rules.
    set(ZLIB_BUILD_SHARED  OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_STATIC  ON  CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(ZLIB_INSTALL       OFF CACHE BOOL "" FORCE)
    add_subdirectory("${ZLIB_DIR_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/zlib" EXCLUDE_FROM_ALL)

    # libpng calls find_package(ZLIB REQUIRED). Point FindZLIB at the zlib built above: with
    # ZLIB::ZLIB already defined it uses that target instead of creating an imported one.
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    set(ZLIB_INCLUDE_DIR "${ZLIB_DIR_SRC}" CACHE PATH "" FORCE)
    set(ZLIB_LIBRARY ZLIB::ZLIB CACHE STRING "" FORCE)

    # libpng: static library only, no tests, tools or install rules.
    set(PNG_SHARED    OFF CACHE BOOL "" FORCE)
    set(PNG_STATIC    ON  CACHE BOOL "" FORCE)
    set(PNG_FRAMEWORK OFF CACHE BOOL "" FORCE)
    set(PNG_TESTS     OFF CACHE BOOL "" FORCE)
    set(PNG_TOOLS     OFF CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL ON)
    add_subdirectory("${LIBPNG_DIR_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/libpng" EXCLUDE_FROM_ALL)

    set(HEXSTARTUP_PNG_LIBS png_static)
    message(STATUS "HexStartup: building libpng/zlib from the submodules in external/")
endif()

# --------------------------------------------------------------------------------------------
# HexStartup
# --------------------------------------------------------------------------------------------
add_executable(HexStartup
    src/Source.cpp
    src/Interactive.cpp
    src/Image.cpp
    src/Hexen.cpp
    src/PPM.cpp
    src/_PNG.cpp
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

# --------------------------------------------------------------------------------------------
# Tests (-DBUILD_TESTING=OFF to skip)
# --------------------------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
