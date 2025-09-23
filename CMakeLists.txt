# CMake version configuration
# (Note: The minimal version 3.22 is shipped with Ubuntu 22.04)
cmake_minimum_required(VERSION 3.22)

# Fix missing libOpenGL.so.0 error of binary Linux releases, see
# https://github.com/probonopd/linuxdeployqt/issues/486.
set(OpenGL_GL_PREFERENCE LEGACY)

# WARNING:
# Read the release workflow documentation (at https://developers.librepcb.org)
# before making changes to the version numbers!

# Application version:
#  - Always three numbers (MAJOR.MINOR.PATCH)!
#  - Unstable (development) versions: Suffix "-unstable", e.g. "1.0.0-unstable"
#  - Release candidates: Suffix "-rc#", e.g. "1.0.0-rc3"
#  - Releases: No suffix, e.g. "1.0.0"
set(LIBREPCB_APP_VERSION "1.3.1-unstable")

# File format version:
#  - Must be equal to the major version of APP_VERSION!
#  - If APP_VERSION < 1.0.0: Two numbers, e.g. "0.2" for APP_VERSION=="0.2.x"
#  - If APP_VERSION >= 1.0.0: Only one number, e.g. "2" for APP_VERSION=="2.x.y"
set(LIBREPCB_FILE_FORMAT_VERSION "1")

# File format stable flag:
#  - On all non-release branches: false
#  - On release branches: true
# Note: Do not use a CMake boolean (ON / TRUE), use a C++ style boolean
# string instead ("true" / "false").
set(LIBREPCB_FILE_FORMAT_STABLE "true")

# Define project
project(librepcb LANGUAGES CXX)

# Configure module path
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Global options
option(BUILD_TESTS "Build unit tests." ON)
option(BUILD_DISALLOW_WARNINGS
       "Disallow compiler warnings during build (build with -Werror)." OFF
)
option(UNBUNDLE_DXFLIB "Don't use vendored dxflib library." OFF)
option(UNBUNDLE_FONTOBENE_QT "Don't use vendored FontoBeneQt library." OFF)
option(UNBUNDLE_GTEST "Don't use vendored GoogleTest library." OFF)
option(UNBUNDLE_MUPARSER "Don't use vendored MuParser library." OFF)
option(UNBUNDLE_POLYCLIPPING "Don't use vendored Polyclipping library." OFF)
option(UNBUNDLE_ALL "Don't use any vendored library." OFF)
option(USE_GLU "Include features depending on OpenGL Utility Library (GLU)." ON)
option(USE_OPENCASCADE "Include features depending on OpenCascade." ON)

# Create a release build by default
include(DefaultBuildType)

# Force static linking for local libs
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "Link dynamically against local libraries." FORCE
)

# Auto-include current directory
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Use Qt6
set(QT_MAJOR_VERSION 6)
set(QT "Qt${QT_MAJOR_VERSION}")

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include GNU install dirs variables
include(GNUInstallDirs)

# Create "common" interface library which is used for DRY configuration of
# build flags.
add_library(common INTERFACE)
target_compile_options(common INTERFACE -Wall -Wextra)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Slint generated code leads to warnings which we can suppress only for Clang,
  # see https://github.com/slint-ui/slint/issues/2681
  target_compile_options(common INTERFACE -Wpedantic)
endif()
if(BUILD_DISALLOW_WARNINGS)
  target_compile_options(common INTERFACE -Werror)
endif()

# GCC 13.x emits a false-positive (?) warning.
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU
   AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0
)
  target_compile_options(common INTERFACE -Wno-maybe-uninitialized)
endif()

# The TypeSafe library causes some compiler warnings which are not of
# interest, so let's suppress them.
target_compile_options(common INTERFACE -Wno-noexcept-type)

# Avoid "file too big" error for debug builds on Windows.
if(WIN32)
  target_compile_options(common INTERFACE $<$<CONFIG:Debug>:-Wa,-mbig-obj>)
endif()

# Enforce UNICODE on Windows to avoid an incompatibility with OpenCascade.
if(WIN32)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

# Disable Qt functions which are deprecated in Qt <= 6.2 to enforce us
# migrating away from them. Always set this to the lowest Qt version we want
# to support.
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060200)

# Not sure what we should do with deprecation warnings. Better keeping
# backwards compatibility with already existing & supported environments
# instead of forward compatibility with not yet existing environments, no?
add_definitions(
  -DQT_NO_DEPRECATED_WARNINGS # Qt
  -DGL_SILENCE_DEPRECATION # OpenGL
  -DOCCT_NO_DEPRECATED # OpenCascade
)

# Determine required Qt components
set(QT_COMPONENTS
    Concurrent
    Core
    Gui
    LinguistTools
    Network
    OpenGL
    OpenGLWidgets
    PrintSupport
    Sql
    Svg
    Widgets
)
if(BUILD_TESTS)
  list(APPEND QT_COMPONENTS Test)
endif()

# Find Qt
find_package(${QT} 6.2.0 REQUIRED COMPONENTS ${QT_COMPONENTS})
set(QT_VERSION "${${QT}_VERSION}")
message(STATUS "Building LibrePCB with Qt ${QT_VERSION}")

# For the ParsEagle library, use the same Qt version as for LibrePCB
set(PARSEAGLE_QT_MAJOR_VERSION ${QT_MAJOR_VERSION})

# False-positives in Qt 6.8 and 6.9 (maybe even more versions?), see
# https://bugreports.qt.io/browse/QTBUG-135623)
if(QT_VERSION VERSION_GREATER_EQUAL 6.8 AND QT_VERSION VERSION_LESS 6.10)
  target_compile_options(common INTERFACE -Wno-array-bounds)
endif()

# Find third party libraries
find_package(DelaunayTriangulation REQUIRED)
find_package(Dxflib REQUIRED)
find_package(FontoBeneQt REQUIRED)
find_package(GLU REQUIRED)
find_package(MuParser REQUIRED)
find_package(OpenCascade REQUIRED)
find_package(Polyclipping REQUIRED)
find_package(Slint REQUIRED)
find_package(TypeSafe REQUIRED)
if(BUILD_TESTS)
  find_package(GTest REQUIRED)
endif()

# Add libs
add_subdirectory(libs/corrosion)
add_subdirectory(libs/librepcb/core)
add_subdirectory(libs/librepcb/editor)
add_subdirectory(libs/librepcb/eagleimport)
add_subdirectory(libs/librepcb/kicadimport)
add_subdirectory(libs/librepcb/rust-core)
add_subdirectory(libs/parseagle)

# Add apps
add_subdirectory(apps/librepcb)
add_subdirectory(apps/librepcb-cli)

# Add unittests
if(BUILD_TESTS)
  add_subdirectory(tests/unittests)
endif()

# Generate translation file target
# Note: It is not very nice that *.qm files will be generated into the source
# "share" directory. However, this seems the only easy way to get translations
# working in local builds (without "make install") and *should* not hurt much.
set(LIBREPCB_QM_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/share/librepcb/i18n")
file(MAKE_DIRECTORY "${LIBREPCB_QM_FILES_DIR}")
file(
  GLOB LIBREPCB_TS_FILES
  RELATIVE ${CMAKE_SOURCE_DIR}
  i18n/librepcb_*.ts
)
set_source_files_properties(
  ${LIBREPCB_TS_FILES} PROPERTIES OUTPUT_LOCATION "${LIBREPCB_QM_FILES_DIR}"
)
qt6_add_translation(LIBREPCB_QM_FILES ${LIBREPCB_TS_FILES} OPTIONS -silent)
add_custom_target(librepcb_translations DEPENDS ${LIBREPCB_QM_FILES})
add_dependencies(librepcb librepcb_translations)

# Install target for shared files
file(
  GLOB SHARE_DIRS
  LIST_DIRECTORIES true
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  share/*
)
foreach(d ${SHARE_DIRS})
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${d}")
    install(
      DIRECTORY ${d}
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}
      REGEX "/\.(git|gitattributes|gitignore)$" EXCLUDE
    )
  endif()
endforeach()
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/i18n/share/
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR} OPTIONAL
)
install(
  DIRECTORY ${LIBREPCB_QM_FILES_DIR}
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/librepcb
)

# Windows installer configuration
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/dist/innosetup/config.iss.in
  ${CMAKE_BINARY_DIR}/dist/innosetup/config.iss @ONLY
)
add_custom_target(
  librepcb_installer DEPENDS ${CMAKE_BINARY_DIR}/dist/innosetup/config.iss
)
add_dependencies(librepcb librepcb_installer)
