set(GTEST_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/googletest")
if(EXISTS "${GTEST_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_GTEST
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored GoogleTest / GoogleMock")

  # Include local submodule
  add_subdirectory(
    "${GTEST_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/googletest"
    EXCLUDE_FROM_ALL
  )

  # Aliases to namespaced variant
  add_library(GTest::GTest ALIAS gtest)
  add_library(GTest::GMock ALIAS gmock)

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system
# Note: GMock might be contained within the GTest package, so we don't fail
#       if the GMock package was not found. We just try to create the alias.

find_package(GTest CONFIG)
find_package(GMock CONFIG)
if(GTest_FOUND)
  message(STATUS "Using system GoogleTest / GoogleMock")

  # Add uppercase alias if only the lowercase target is defined
  if(NOT TARGET GTest::GTest)
    add_library(GTest::GTest ALIAS GTest::gtest)
  endif()
  if(NOT TARGET GTest::GMock)
    add_library(GTest::GMock ALIAS GTest::gmock)
  endif()

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system via pkg-config
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(GTest GLOBAL IMPORTED_TARGET gtest)
  pkg_check_modules(GMock GLOBAL IMPORTED_TARGET gmock)
endif()
if(GTest_FOUND)
  message(STATUS "Using system GTest / GMock (via pkg-config)")
  add_library(GTest::GTest ALIAS PkgConfig::GTest)
  add_library(GTest::GMock ALIAS PkgConfig::GMock)
  return()
endif()

message(FATAL_ERROR "Did not find GoogleTest / GoogleMock system libraries")
