# CMake version configuration
# (Note: The minimal version 3.5 is shipped with Ubuntu 16.04)
cmake_minimum_required(VERSION 3.5...3.19)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Support older macOS versions. Needs to be set before calling project()!
# https://github.com/LibrePCB/LibrePCB/issues/1091
set(CMAKE_OSX_DEPLOYMENT_TARGET
    "10.14"
    CACHE STRING "Minimum OS X deployment version"
)

# WARNING:
# Read the release workflow documentation (at https://developers.librepcb.org)
# before making changes to the version numbers!

# Application version:
#  - Always three numbers (MAJOR.MINOR.PATCH)!
#  - Unstable versions (non-release branches): Suffix "-unstable", e.g.
#    "1.0.0-unstable"
#  - Release candidates (on release branches): Suffix "-rc#", e.g. "1.0.0-rc3"
#  - Releases (on release branches): No suffix, e.g. "1.0.0"
set(LIBREPCB_APP_VERSION "0.2.0-unstable")

# File format version:
#  - Must be equal to the major version of APP_VERSION!
#  - If APP_VERSION < 1.0.0: Two numbers, e.g. "0.2" for APP_VERSION=="0.2.x"
#  - If APP_VERSION >= 1.0.0: Only one number, e.g. "2" for APP_VERSION=="2.x.y"
set(LIBREPCB_FILE_FORMAT_VERSION "0.2")

# File format stable flag:
#  - On all non-release branches: false
#  - On release branches: true
# Note: Do not use a CMake boolean (ON / TRUE), use a C++ style boolean
# string instead ("true" / "false").
set(LIBREPCB_FILE_FORMAT_STABLE "false")

# Define project
project(librepcb LANGUAGES CXX)

# Configure module path
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Global options
option(BUILD_TESTS "Build unit tests." ON)
option(BUILD_DISALLOW_WARNINGS
       "Disallow compiler warnings during build (build with -Werror)." OFF
)
option(BUILD_QTQUICK_TEST "Build the QML test window." ON)
option(UNBUNDLE_DXFLIB "Don't use vendored dxflib library." OFF)
option(UNBUNDLE_FONTOBENE_QT5 "Don't use vendored FontoBeneQt5 library." OFF)
option(UNBUNDLE_GTEST "Don't use vendored GoogleTest library." OFF)
option(UNBUNDLE_HOEDOWN "Don't use vendored Hoedown library." OFF)
option(UNBUNDLE_MUPARSER "Don't use vendored MuParser library." OFF)
option(UNBUNDLE_POLYCLIPPING "Don't use vendored Polyclipping library." OFF)
option(UNBUNDLE_QUAZIP "Don't use vendored QuaZip library." OFF)
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

# Use C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include GNU install dirs variables
include(GNUInstallDirs)

# Workaround for builds on macOS with Qt installed through homebrew
# https://github.com/Homebrew/homebrew-core/issues/8392#issuecomment-325226494
if(APPLE AND EXISTS /usr/local/opt/qt5)
  # Homebrew installs Qt5 (up to at least 5.9.1) in
  # /usr/local/qt5, ensure it can be found by CMake since
  # it is not in the default /usr/local prefix.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

# Create "common" interface library which is used for DRY configuration of
# build flags.
add_library(common INTERFACE)
target_compile_options(common INTERFACE -Wall -Wextra -Wpedantic)
if(BUILD_DISALLOW_WARNINGS)
  target_compile_options(common INTERFACE -Werror)
endif()

# The TypeSafe library causes some compiler warnings which are not of
# interest, so let's suppress them.
target_compile_options(common INTERFACE -Wno-noexcept-type)

# Clang emits some warnings within the muParser library when compiling with
# -Wpedantic, so let's suppress these warnings when compiling with Clang.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(common INTERFACE -Wno-nested-anon-types)
endif()

# Enforce UNICODE on Windows to avoid an incompatibility with OpenCascade.
if(WIN32)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

# Find required Qt packages
find_package(
  Qt5 5.5.0
  COMPONENTS Concurrent
             Gui
             LinguistTools
             Network
             OpenGL
             PrintSupport
             Sql
             Svg
             Widgets
  REQUIRED
)
if(BUILD_QTQUICK_TEST)
  find_package(
    Qt5 5.5.0
    COMPONENTS Quick QuickControls2
    REQUIRED
  )
endif()
if(BUILD_TESTS)
  find_package(
    Qt5 5.5.0
    COMPONENTS Test
    REQUIRED
  )
endif()
message(STATUS "Building with Qt${Qt5Core_VERSION}")

# In Qt 5.15, a lot of things were marked as deprecated, without providing
# alternatives available in previous Qt versions. It would require a lot of
# preprocessor conditionals to avoid these deprecation warnings, so let's just
# disable them for now. We are using CI anyway to ensure LibrePCB compiles with
# the targeted Qt versions.
if("${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}" VERSION_GREATER 5.14)
  target_compile_options(common INTERFACE -Wno-deprecated-declarations)
endif()

# Find third party libraries
find_package(DelaunayTriangulation REQUIRED)
find_package(Dxflib REQUIRED)
find_package(FontoBeneQt5 REQUIRED)
find_package(GLU REQUIRED)
find_package(MuParser REQUIRED)
find_package(OpenCascade REQUIRED)
find_package(Optional REQUIRED)
find_package(Polyclipping REQUIRED)
find_package(QuaZip REQUIRED)
find_package(TypeSafe REQUIRED)
if(BUILD_TESTS)
  find_package(GTest REQUIRED)
endif()
# Hoedown is only needed on Qt <5.14
if(Qt5Core_VERSION VERSION_LESS 5.14)
  message(STATUS "Qt <5.14 detected, using Hoedown for markdown support")
  find_package(Hoedown REQUIRED)
endif()

# Add libs
add_subdirectory(libs/librepcb/core)
add_subdirectory(libs/librepcb/editor)
add_subdirectory(libs/librepcb/eagleimport)
add_subdirectory(libs/parseagle)

# Add apps
add_subdirectory(apps/librepcb)
add_subdirectory(apps/librepcb-cli)

# Add unittests
if(BUILD_TESTS)
  add_subdirectory(tests/unittests)
endif()

# Generate translation file target
set(LIBREPCB_QM_FILES_DIR "${CMAKE_BINARY_DIR}/i18n")
file(MAKE_DIRECTORY "${LIBREPCB_QM_FILES_DIR}")
file(
  GLOB LIBREPCB_TS_FILES
  RELATIVE ${CMAKE_SOURCE_DIR}
  i18n/librepcb_*.ts
)
set_source_files_properties(
  ${LIBREPCB_TS_FILES} PROPERTIES OUTPUT_LOCATION "${LIBREPCB_QM_FILES_DIR}"
)
if(Qt5_VERSION VERSION_LESS 5.12)
  # Qt <5.12 doesn't support OPTIONS
  qt5_add_translation(LIBREPCB_QM_FILES ${LIBREPCB_TS_FILES})
else()
  qt5_add_translation(LIBREPCB_QM_FILES ${LIBREPCB_TS_FILES} OPTIONS -silent)
endif()
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
  DIRECTORY ${CMAKE_BINARY_DIR}/i18n
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/librepcb
)
