# include user-defined things in every qmake project
exists(custom.pri):include(custom.pri)

# set destination path for generated files
isEmpty(GENERATED_DIR):GENERATED_DIR = generated

# set destination path for binaries and libraries
macx:DESTDIR = $${GENERATED_DIR}/mac
unix:!macx:DESTDIR = $${GENERATED_DIR}/unix
win32:DESTDIR = $${GENERATED_DIR}/windows

# use separate folders for different types of files
OBJECTS_DIR = obj
MOC_DIR = moc
RCC_DIR = rcc
UI_DIR = ui
UI_HEADERS_DIR = ui
UI_SOURCES_DIR = ui

# Qt >= 5.2 is obligatory!
lessThan(QT_MAJOR_VERSION, 5) {
    error("Qt version $$[QT_VERSION] is too old, should be version 5.2 or newer!")
} else {
    lessThan(QT_MINOR_VERSION, 2) {
        error("Qt version $$[QT_VERSION] is too old, should be version 5.2 or newer!")
    }
}

# c++11 is obligatory!
CONFIG += c++11
