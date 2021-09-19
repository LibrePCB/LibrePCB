# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Executable
add_executable(
  librepcb MACOSX_BUNDLE # When building on macOS, create a bundle
  ../../img/images.qrc
  controlpanel/controlpanel.cpp
  controlpanel/controlpanel.h
  controlpanel/controlpanel.ui
  firstrunwizard/firstrunwizard.cpp
  firstrunwizard/firstrunwizard.h
  firstrunwizard/firstrunwizard.ui
  firstrunwizard/firstrunwizardpage_welcome.cpp
  firstrunwizard/firstrunwizardpage_welcome.h
  firstrunwizard/firstrunwizardpage_welcome.ui
  firstrunwizard/firstrunwizardpage_workspacepath.cpp
  firstrunwizard/firstrunwizardpage_workspacepath.h
  firstrunwizard/firstrunwizardpage_workspacepath.ui
  initializeworkspacewizard/initializeworkspacewizard.cpp
  initializeworkspacewizard/initializeworkspacewizard.h
  initializeworkspacewizard/initializeworkspacewizard.ui
  initializeworkspacewizard/initializeworkspacewizard_chooseimportversion.cpp
  initializeworkspacewizard/initializeworkspacewizard_chooseimportversion.h
  initializeworkspacewizard/initializeworkspacewizard_chooseimportversion.ui
  initializeworkspacewizard/initializeworkspacewizard_choosesettings.cpp
  initializeworkspacewizard/initializeworkspacewizard_choosesettings.h
  initializeworkspacewizard/initializeworkspacewizard_choosesettings.ui
  initializeworkspacewizard/initializeworkspacewizard_finalizeimport.cpp
  initializeworkspacewizard/initializeworkspacewizard_finalizeimport.h
  initializeworkspacewizard/initializeworkspacewizard_finalizeimport.ui
  initializeworkspacewizard/initializeworkspacewizardcontext.cpp
  initializeworkspacewizard/initializeworkspacewizardcontext.h
  main.cpp
  markdown/markdownconverter.cpp
  markdown/markdownconverter.h
  projectlibraryupdater/projectlibraryupdater.cpp
  projectlibraryupdater/projectlibraryupdater.h
  projectlibraryupdater/projectlibraryupdater.ui
)
target_include_directories(
  librepcb
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  librepcb
  PRIVATE common
          # LibrePCB
          LibrePCB::LibraryManager
          LibrePCB::LibraryEditor
          LibrePCB::ProjectEditor
          LibrePCB::Workspace
          LibrePCB::Project
          LibrePCB::Library
          LibrePCB::Common
          # Third party
          Optional::Optional
          TypeSafe::TypeSafe
          Polyclipping::Polyclipping
          # Qt
          Qt5::Gui
          Qt5::Widgets
)

# Hoedown is only needed on Qt <5.14
if(Qt5Core_VERSION_MAJOR EQUAL 5 AND Qt5Core_VERSION_MINOR LESS 14)
  target_link_libraries(librepcb PRIVATE Hoedown::Hoedown)
endif()

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
    librepcb
    PROPERTIES BUNDLE True
               MACOSX_BUNDLE_GUI_IDENTIFIER org.librepcb.LibrePCB
               MACOSX_BUNDLE_BUNDLE_NAME LibrePCB
               MACOSX_BUNDLE_BUNDLE_VERSION ${LIBREPCB_APP_VERSION}
               MACOSX_BUNDLE_SHORT_VERSION_STRING ${LIBREPCB_APP_VERSION}
               MACOSX_BUNDLE_ICON_FILE librepcb.icns
               MACOSX_BUNDLE_INFO_STRING
               "A new, powerful and intuitive EDA tool for everyone."
               MACOSX_BUNDLE_COPYRIGHT
               "© LibrePCB contributors, released under the GPLv3 license"
               MACOSX_BUNDLE_INFO_PLIST
               "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
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
