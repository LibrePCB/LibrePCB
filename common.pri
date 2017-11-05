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

# set destination path to share directory
SHARE_DIR = $$shadowed($$absolute_path($${GENERATED_DIR}/share, $$_PRO_FILE_PWD_))

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

# redirect qInfo to qDebug for Qt < 5.5 because qInfo was not yet available
# https://doc.qt.io/qt-5/qtglobal.html#qInfo
lessThan(QT_MINOR_VERSION, 5) {
    DEFINES += qInfo=qDebug
}

# do not allow to use -Werror in release mode on Qt < 5.5 because Q_ASSERT() would lead
# to warnings (resp. errors) if the argument of Q_ASSERT() is not used elsewhere.
#   --> see: http://code.qt.io/cgit/qt/qtbase.git/tree/dist/changes-5.5.0
CONFIG(release, debug|release) {
    lessThan(QT_MINOR_VERSION, 5) {
        QMAKE_CFLAGS -= -Werror
        QMAKE_CXXFLAGS -= -Werror
    }
}

# c++11 is obligatory!
CONFIG += c++11

# enable compiler warnings
CONFIG += warn_on
QMAKE_CXXFLAGS += -Wextra
QMAKE_CXXFLAGS_DEBUG += -Wextra

# QuaZIP: use as static library
DEFINES += QUAZIP_STATIC
