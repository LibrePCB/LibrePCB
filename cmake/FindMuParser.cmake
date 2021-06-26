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

find_package(muparser 2.0 REQUIRED)

if(muparser_FOUND)
    message(STATUS "Using system MuParser")

    # Stop here, we're done
    return()
endif()

message(FATAL_ERROR "Did not find MuParser system library")

# Note: muParser hasn't made a release yet with the cmake changes that would
# enable the find_package call above. Until then, we could add another approach
# to detect the library, using pkg-config (TODO).
