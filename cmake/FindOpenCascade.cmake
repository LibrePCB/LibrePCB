# Add library
add_library(open_cascade INTERFACE)
add_library(OpenCascade::OpenCascade ALIAS open_cascade)

# Leave library empty when building without OpenCascade
if(NOT USE_OPENCASCADE)
  message(STATUS "Building without OpenCascade features")
  set(OCC_EDITION_NAME "N/A") # Referenced from occmodel.cpp
  return()
endif()

# Try to find OCCT shared library on the system
find_package(OpenCASCADE CONFIG QUIET)
if(OpenCASCADE_FOUND)
  message(STATUS "Using system OpenCASCADE (OCCT ${OpenCASCADE_VERSION})")

  # Specify OpenCascade libraries needed for LibrePCB
  # https://github.com/LibrePCB/LibrePCB/issues/1315
  if(OpenCASCADE_VERSION VERSION_GREATER 7.8.0)
    set(OCC_LIBRARIES TKCAF TKDESTEP)
  elseif(OpenCASCADE_VERSION VERSION_EQUAL 7.8.0)
    message(
      FATAL_ERROR
        "Cannot use OpenCascade 7.8.0 because it contains a critical bug. \
        Please use a different version (or if this is not possible, try OCE \
        instead of OCCT)."
    )
  else()
    set(OCC_LIBRARIES TKXCAF TKXDESTEP)
  endif()

  # Populate target
  target_include_directories(
    open_cascade SYSTEM INTERFACE "${OpenCASCADE_INCLUDE_DIR}"
  )
  find_library(OCC_LIBRARIES, PATHS "${OpenCASCADE_LIBRARY_DIR}")
  target_link_libraries(open_cascade INTERFACE ${OCC_LIBRARIES})
  set(OCC_EDITION_NAME "OCCT") # Referenced from occmodel.cpp

  # Stop here, we're done
  return()
endif()

# Try to find OCE shared library on the system
find_package(OCE CONFIG QUIET)
if(OCE_FOUND)
  message(STATUS "Using system OpenCASCADE (OCE ${OCE_VERSION})")

  # Specify OpenCascade libraries needed for LibrePCB
  set(OCC_LIBRARIES TKXCAF TKXDESTEP)

  # Populate target
  target_include_directories(
    open_cascade SYSTEM INTERFACE "${OCE_INCLUDE_DIRS}"
  )
  find_library(OCC_LIBRARIES, PATHS "${OCE_LIBRARIES}")
  target_link_libraries(open_cascade INTERFACE ${OCC_LIBRARIES})
  set(OCC_EDITION_NAME "OCE") # Referenced from occmodel.cpp

  # Stop here, we're done
  return()
endif()

message(FATAL_ERROR "Did not find OpenCASCADE library! \
    Consider passing '-DUSE_OPENCASCADE=0' to cmake."
)
