# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Executable
add_executable(
  librepcb
  # When building on Windows, mark this as a GUI application to hide cmd.exe
  WIN32
  # When building on macOS, create a bundle
  MACOSX_BUNDLE
  # Resources
  ../../img/images.qrc
  # Sources
  apptoolbox.cpp
  apptoolbox.h
  guiapplication.cpp
  guiapplication.h
  library/librariesmodel.cpp
  library/librariesmodel.h
  main.cpp
  mainwindow.cpp
  mainwindow.h
  project/projecteditor.cpp
  project/projecteditor.h
  project/projectsmodel.cpp
  project/projectsmodel.h
  project/schematiceditor.cpp
  project/schematiceditor.h
  workspace/favoriteprojectsmodel.cpp
  workspace/favoriteprojectsmodel.h
  workspace/filesystemmodel.cpp
  workspace/filesystemmodel.h
  workspace/recentprojectsmodel.cpp
  workspace/recentprojectsmodel.h
)
slint_target_sources(librepcb ui/appwindow.slint NAMESPACE ui)
target_include_directories(
  librepcb
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  librepcb
  PRIVATE common
          # LibrePCB
          LibrePCB::Editor
          LibrePCB::Core
          # Third party
          Slint::Slint
          # Qt
          ${QT}::Core
          ${QT}::Gui
          ${QT}::Widgets
)

# Install target
install(
  TARGETS librepcb
  BUNDLE DESTINATION "${CMAKE_INSTALL_BINDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# macOS bundle configuration
if(APPLE)
  # Add icon
  set(MACOSX_ICON_PATH "${CMAKE_SOURCE_DIR}/img/app/librepcb.icns")
  target_sources(librepcb PUBLIC "${MACOSX_ICON_PATH}")
  set_source_files_properties(
    "${MACOSX_ICON_PATH}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
  )

  # Set bundle properties
  set_target_properties(
    librepcb PROPERTIES BUNDLE True MACOSX_BUNDLE_INFO_PLIST
                                    "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
  )
endif()

# Windows executable configuration
if(WIN32)
  # Add icon
  set(WINDOWS_APP_ICON_RESOURCE
      "${CMAKE_SOURCE_DIR}/cmake/WindowsExecutableResource.rc"
  )
  target_sources(librepcb PUBLIC "${WINDOWS_APP_ICON_RESOURCE}")
endif()
