# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Executable
add_executable(
  uuid_generator
  # When building on Windows, mark this as a GUI application to hide cmd.exe
  WIN32
  main.cpp mainwindow.cpp mainwindow.h mainwindow.ui
)
target_include_directories(
  uuid_generator
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  uuid_generator
  PRIVATE common
          # LibrePCB
          LibrePCB::Common
          # Qt
          Qt5::Gui
          Qt5::Widgets
)
set_target_properties(uuid_generator PROPERTIES OUTPUT_NAME uuid-generator)
