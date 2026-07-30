# Create empty dummy library when building without GLU
if(NOT USE_GLU)
  message(STATUS "Building without OpenGL Utility Library (GLU)")
  if(NOT TARGET opengl_glu)
    add_library(opengl_glu INTERFACE)
    add_library(OpenGL::GLU ALIAS opengl_glu)
  endif()
  set(GLU_FOUND TRUE)
  return()
endif()

# Try to find OpenGL library on the system
find_package(OpenGL QUIET)
if(OpenGL_FOUND)
  set(GLU_FOUND TRUE)
  # Stop here, we're done
  return()
endif()

message(FATAL_ERROR "Did not find OpenGL Utility Library (GLU)! \
    Consider passing '-DUSE_GLU=0' to cmake."
)
