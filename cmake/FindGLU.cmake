# Ignore subsequent calls (https://github.com/LibrePCB/LibrePCB/issues/1812)
if(TARGET OpenGL::GLU)
  return()
endif()

# Create empty dummy library when building without GLU
if(NOT USE_GLU)
  message(STATUS "Building without OpenGL Utility Library (GLU)")
  add_library(opengl_glu INTERFACE)
  add_library(OpenGL::GLU ALIAS opengl_glu)
  set(GLU_FOUND TRUE)
  return()
endif()

# Try to find OpenGL library on the system
find_package(OpenGL QUIET)
if(OpenGL_FOUND)
  # Stop here, we're done
  set(GLU_FOUND TRUE)
  return()
endif()

message(FATAL_ERROR "Did not find OpenGL Utility Library (GLU)! \
    Consider passing '-DUSE_GLU=0' to cmake."
)
