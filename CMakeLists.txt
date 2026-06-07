# libgdbparse — standalone C++ library for parsing Garmin MapSource .gdb files.
#
# Builds a static or shared library that links a minimal subset of the gpsbabel
# 1.9.0 sources required by GdbFormat::read(), and exposes a small POD API
# (gdbparse::parse_gdb).
#
# Cross-platform: tested for Linux + Windows (MSVC/MinGW). Requires Qt5 (>= 5.12)
# or Qt6 Core.
#
# Quick start (from the project root):
#     cmake -S lib_gdbparse -B build_gdbparse
#     cmake --build build_gdbparse -j
#     ./build_gdbparse/gdbparse_demo reference/gdb-sample.gdb

cmake_minimum_required(VERSION 3.16)
project(gdbparse LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Locate gpsbabel source tree. We default to the parent of this directory but
# allow override (e.g. when consumed from a different repo layout).
set(GDBPARSE_GPSBABEL_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH
    "Path to the gpsbabel 1.9.0 source tree")

if(NOT EXISTS "${GDBPARSE_GPSBABEL_DIR}/gdb.cc")
    message(FATAL_ERROR
        "gdb.cc not found in GDBPARSE_GPSBABEL_DIR=${GDBPARSE_GPSBABEL_DIR}; "
        "point this variable at the gpsbabel 1.9.0 source root.")
endif()

# ---------------------------------------------------------------------------
# Qt5 or Qt6 Core.
# ---------------------------------------------------------------------------
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_VERSION} COMPONENTS Core REQUIRED)
set(GDBPARSE_QT_LIBS Qt${QT_VERSION_MAJOR}::Core)
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS Core5Compat REQUIRED)
    list(APPEND GDBPARSE_QT_LIBS Qt6::Core5Compat)
endif()

# gbfile.cc supports transparent .gz I/O — needs zlib.
find_package(ZLIB REQUIRED)

# ---------------------------------------------------------------------------
# Minimum set of gpsbabel sources needed by GdbFormat::read().
# ---------------------------------------------------------------------------
set(GPSBABEL_SOURCES
    ${GDBPARSE_GPSBABEL_DIR}/gdb.cc
    ${GDBPARSE_GPSBABEL_DIR}/gbfile.cc
    ${GDBPARSE_GPSBABEL_DIR}/garmin_fs.cc
    ${GDBPARSE_GPSBABEL_DIR}/garmin_tables.cc
    ${GDBPARSE_GPSBABEL_DIR}/grtcirc.cc
    ${GDBPARSE_GPSBABEL_DIR}/mkshort.cc
    ${GDBPARSE_GPSBABEL_DIR}/formspec.cc
    ${GDBPARSE_GPSBABEL_DIR}/util.cc
    ${GDBPARSE_GPSBABEL_DIR}/route.cc
    ${GDBPARSE_GPSBABEL_DIR}/waypt.cc
    ${GDBPARSE_GPSBABEL_DIR}/globals.cc
    ${GDBPARSE_GPSBABEL_DIR}/session.cc
    ${GDBPARSE_GPSBABEL_DIR}/geocache.cc
    ${GDBPARSE_GPSBABEL_DIR}/inifile.cc
    ${GDBPARSE_GPSBABEL_DIR}/jeeps/gpsmath.cc
    ${GDBPARSE_GPSBABEL_DIR}/src/core/logging.cc
)

# Our own wrapper + replacement for fatal.cc (throws instead of exit).
set(GDBPARSE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/gdbparse.cc
    ${CMAKE_CURRENT_LIST_DIR}/fatal_lib.cc
)

# ---------------------------------------------------------------------------
# Build the library. Default = static; set BUILD_SHARED_LIBS=ON for .so/.dll.
# ---------------------------------------------------------------------------
add_library(gdbparse ${GDBPARSE_SOURCES} ${GPSBABEL_SOURCES})

target_include_directories(gdbparse
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
        ${GDBPARSE_GPSBABEL_DIR}
        ${CMAKE_CURRENT_LIST_DIR}    # gbversion.h stub lives here
)

target_link_libraries(gdbparse
    PUBLIC  ${GDBPARSE_QT_LIBS}
    PRIVATE ZLIB::ZLIB
)

if(MSVC)
    target_compile_definitions(gdbparse PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(gdbparse PRIVATE /wd4244 /wd4267 /wd4996)
else()
    target_compile_options(gdbparse PRIVATE -Wno-deprecated-declarations)
endif()

# ---------------------------------------------------------------------------
# Demo executable: prints a summary of the parsed waypoints/routes/tracks.
# ---------------------------------------------------------------------------
option(GDBPARSE_BUILD_DEMO "Build the gdbparse_demo CLI" ON)
if(GDBPARSE_BUILD_DEMO)
    add_executable(gdbparse_demo ${CMAKE_CURRENT_LIST_DIR}/demo.cc)
    target_link_libraries(gdbparse_demo PRIVATE gdbparse)
endif()
