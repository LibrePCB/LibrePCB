# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Executable
add_executable(
  librepcb_mcp MACOSX_BUNDLE # When building on macOS, create a bundle
  main.cpp
)
target_include_directories(
  librepcb_mcp
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  librepcb_mcp
  PRIVATE common
          # LibrePCB
          LibrePCB::Automation
          LibrePCB::Core
          # Qt
          ${QT}::Core
          ${QT}::Widgets
)
set_target_properties(librepcb_mcp PROPERTIES OUTPUT_NAME librepcb-mcp)

# Install target
install(
  TARGETS librepcb_mcp
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
  target_sources(librepcb_mcp PUBLIC "${MACOSX_ICON_PATH}")
  set_source_files_properties(
    "${MACOSX_ICON_PATH}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
  )

  # Set bundle properties (used for Info.plist)
  set_target_properties(
    librepcb_mcp
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
  target_sources(librepcb_mcp PUBLIC "${WINDOWS_APP_ICON_RESOURCE}")
endif()
