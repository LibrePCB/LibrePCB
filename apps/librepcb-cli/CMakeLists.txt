# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Executable
add_executable(
  librepcb_cli MACOSX_BUNDLE # When building on macOS, create a bundle
  commandlineinterface.cpp commandlineinterface.h main.cpp
)
target_include_directories(
  librepcb_cli
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  librepcb_cli
  PRIVATE common
          # LibrePCB
          LibrePCB::Core
          # Qt
          ${QT}::Core
)
set_target_properties(librepcb_cli PROPERTIES OUTPUT_NAME librepcb-cli)

# Install target
install(
  TARGETS librepcb_cli
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
  target_sources(librepcb_cli PUBLIC "${MACOSX_ICON_PATH}")
  set_source_files_properties(
    "${MACOSX_ICON_PATH}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
  )

  # Set bundle properties (used for Info.plist)
  set_target_properties(
    librepcb_cli
    PROPERTIES BUNDLE True MACOSX_BUNDLE_INFO_PLIST
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
