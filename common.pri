# include user-defined things in every qmake project
exists(custom.pri):include(custom.pri)

# set prefix for "make install"
isEmpty(PREFIX):PREFIX = /usr/local

# set destination path for generated files
DESTDIR = $$relative_path($$shadowed("$$PWD/output"), $$OUT_PWD)

# determine absolute path to important directories
OUTPUT_DIR_ABS = $$shadowed($$absolute_path($${DESTDIR}, $${_PRO_FILE_PWD_}))
SHARE_DIR_ABS = $$absolute_path("share", $${PWD})

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

# In Qt 5.15, a lot of things were marked as deprecated, without providing
# alternatives available in previous Qt versions. It would require a lot of
# preprocessor conditionals to avoid these deprecation warnings, so let's just
# disable them for now. We are using CI anyway to ensure LibrePCB compiles with
# the targeted Qt versions.
equals(QT_MAJOR_VERSION, 5) {
    greaterThan(QT_MINOR_VERSION, 14) {
        QMAKE_CXXFLAGS += -Wno-deprecated-declarations
        QMAKE_CXXFLAGS_DEBUG += -Wno-deprecated-declarations
    }
}

# c++11 is obligatory!
CONFIG += c++11

# enable compiler warnings
CONFIG += warn_on
QMAKE_CXXFLAGS += -Wextra
QMAKE_CXXFLAGS_DEBUG += -Wextra

# enable pkgconfig on linux when unbundling
!isEmpty(UNBUNDLE) {
    unix:CONFIG += link_pkgconfig
}

# If UNBUNDLE contains "all", unbundle all dependencies
contains(UNBUNDLE, all) {
    UNBUNDLE = (all) quazip polyclipping fontobene-qt5
}

# QuaZIP configuration
contains(UNBUNDLE, quazip) {
    DEFINES += SYSTEM_QUAZIP
} else:isEmpty(UNBUNDLE) {
    DEFINES += QUAZIP_STATIC
}
