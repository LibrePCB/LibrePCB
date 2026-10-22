# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Executable
add_executable(
  librepcb_mcp
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
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
