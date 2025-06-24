# Set compile options
set(SLINT_FEATURE_BACKEND_QT ON)
set(SLINT_FEATURE_BACKEND_WINIT OFF)
set(SLINT_FEATURE_GETTEXT OFF)
set(SLINT_FEATURE_SYSTEM_TESTING ON)
set(SLINT_FEATURE_EXPERIMENTAL ON) # Required for system testing
set(DEFAULT_SLINT_EMBED_RESOURCES embed-files)
set(SLINT_STYLE "cosmic-dark")

# Include local submodule
add_subdirectory(
  "${PROJECT_SOURCE_DIR}/libs/slint/api/cpp" "${CMAKE_BINARY_DIR}/libs/slint"
)

# Suppress compiler warning (https://github.com/slint-ui/slint/issues/2681)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(Slint INTERFACE -Wno-gnu-anonymous-struct)
endif()

# Suppress compiler warning (https://github.com/slint-ui/slint/issues/7358)
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION
                                          VERSION_LESS 14
)
  target_compile_options(Slint INTERFACE -Wno-maybe-uninitialized)
endif()

# Suppress compiler warning
# (maybe https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98646)
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION
                                          VERSION_LESS 12
)
  target_compile_options(Slint INTERFACE -Wno-nonnull)
endif()

# Suppress compiler warning (https://github.com/slint-ui/slint/issues/7795)
target_compile_options(Slint INTERFACE -Wno-extra-semi)
