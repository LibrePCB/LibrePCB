set(GTEST_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/googletest")
if(EXISTS "${GTEST_SUBMODULE_BASEPATH}" AND NOT UNBUNDLE_GTEST AND NOT UNBUNDLE_ALL)
    message(STATUS "Using vendored GoogleTest / GoogleMock")

    # Include local submodule
    add_subdirectory(
        "${GTEST_SUBMODULE_BASEPATH}"
        "${CMAKE_BINARY_DIR}/libs/googletest"
        EXCLUDE_FROM_ALL
    )

    # Aliases to namespaced variant
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::GMock ALIAS gmock)

    # Stop here, we're done
    return()
endif()

# Otherwise, try to find shared library on the system

find_package(GTest)
find_package(GMock)
if(GTest_FOUND AND GMock_FOUND)
    message(STATUS "Using system GoogleTest / GoogleMock")

    # Stop here, we're done
    return()
endif()

message(FATAL_ERROR "Did not find GoogleTest / GoogleMock system libraries")
