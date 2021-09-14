set(MUPARSER_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/muparser")
if(EXISTS "${MUPARSER_SUBMODULE_BASEPATH}" AND NOT UNBUNDLE_MUPARSER AND NOT UNBUNDLE_ALL)
    message(STATUS "Using vendored MuParser")

    # Disable unneeded features
    set(ENABLE_SAMPLES OFF CACHE BOOL "Build the samples" FORCE)
    set(ENABLE_OPENMP OFF CACHE BOOL "Enable OpenMP for multithreading" FORCE)
    set(ENABLE_WIDE_CHAR OFF CACHE BOOL "Enable wide character support" FORCE)

    # Include local submodule
    add_subdirectory(
        "${MUPARSER_SUBMODULE_BASEPATH}"
        "${CMAKE_BINARY_DIR}/libs/muparser"
        EXCLUDE_FROM_ALL
    )

    # Alias lib to namespaced variant
    add_library(MuParser::MuParser ALIAS muparser)

    # Stop here, we're done
    return()
endif()

# Otherwise, try to find shared library on the system
find_package(muparser 2.0 QUIET)
if(muparser_FOUND)
    message(STATUS "Using system MuParser")

    # Stop here, we're done
    return()
endif()

# Otherwise, try to find shared library on the system via pkg-config
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
    pkg_check_modules(MuParser GLOBAL IMPORTED_TARGET muparser)
endif()
if(MuParser_FOUND)
    message(STATUS "Using system MuParser (via pkg-config)")
    add_library(MuParser::MuParser ALIAS PkgConfig::MuParser)
    return()
endif()

message(FATAL_ERROR "Did not find MuParser system library")
