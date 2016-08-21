# include user-defined things in every qmake project
exists(custom.pri):include(custom.pri)

# set prefix for "make install"
isEmpty(PREFIX):PREFIX = /usr/local

# set destination path for generated files
isEmpty(GENERATED_DIR):GENERATED_DIR = generated

# set destination path for binaries and libraries
macx:DESTDIR = $${GENERATED_DIR}/mac
unix:!macx:DESTDIR = $${GENERATED_DIR}/unix
win32:DESTDIR = $${GENERATED_DIR}/windows

# set destination path for resources
LOCAL_RESOURCES_DIR = $$shadowed($$absolute_path($${GENERATED_DIR}/res, $$_PRO_FILE_PWD_))
INSTALLED_RESOURCES_DIR = $${PREFIX}/share/librepcb/res

# use separate folders for different types of files
OBJECTS_DIR = obj
MOC_DIR = moc
RCC_DIR = rcc
UI_DIR = ui
UI_HEADERS_DIR = ui
UI_SOURCES_DIR = ui

# is qt version sufficient
lessThan(QT_MAJOR_VERSION, 5) {
    error("Qt version $$[QT_VERSION] is too old, should be version 5.2 or newer!")
} else {
    lessThan(QT_MINOR_VERSION, 2) {
        error("Qt version $$[QT_VERSION] is too old, should be version 5.2 or newer!")
    }
}

# c++11 is obligatory!
CONFIG += c++11

# enable compiler warnings
CONFIG += warn_on
QMAKE_CXXFLAGS += -Wextra
QMAKE_CXXFLAGS_DEBUG += -Wextra
