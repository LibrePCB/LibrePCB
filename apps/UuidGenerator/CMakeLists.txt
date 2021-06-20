# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Executable
add_executable(uuid_generator
    # Sources
    main.cpp
    mainwindow.cpp

    # Headers
    mainwindow.h

    # Forms
    mainwindow.ui
)
target_include_directories(uuid_generator PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs")
target_link_libraries(uuid_generator PRIVATE
    common

    # LibrePCB
    LibrePCB::Common

    # Qt
    Qt5::Gui
    Qt5::Widgets
)
set_target_properties(uuid_generator PROPERTIES OUTPUT_NAME uuid-generator)
