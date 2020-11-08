#-------------------------------------------------
# App: LibrePCB command-line interface
#-------------------------------------------------

TEMPLATE = app
TARGET = librepcb-cli

# Use common project definitions
include(../../common.pri)

QT += core widgets opengl network xml printsupport sql

CONFIG += console

# Files to be installed by "make install"
target.path = $${PREFIX}/bin
INSTALLS += target

# Note: The order of the libraries is very important for the linker!
# Another order could end up in "undefined reference" errors!
# Note that dependencies need to be listed *after* the dependent libs.
LIBS += \
    -L$${DESTDIR} \
    -llibrepcblibrarymanager \
    -llibrepcblibraryeditor \
    -llibrepcbprojecteditor \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \
    -llibrepcbcommon \
    -lmuparser \

# Solaris based systems need to link against libproc
solaris:LIBS += -lproc

INCLUDEPATH += \
    ../../libs \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/librepcb/librarymanager \
    ../../libs/librepcb/projecteditor \
    ../../libs/librepcb/libraryeditor \
    ../../libs/librepcb/workspace \
    ../../libs/librepcb/project \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/muparser \

PRE_TARGETDEPS += \
    $${DESTDIR}/libmuparser.a \

isEmpty(UNBUNDLE) {
    # These libraries will only be linked statically when not unbundling
    PRE_TARGETDEPS += \
        $${DESTDIR}/liblibrepcblibrarymanager.a \
        $${DESTDIR}/liblibrepcbprojecteditor.a \
        $${DESTDIR}/liblibrepcblibraryeditor.a \
        $${DESTDIR}/liblibrepcbworkspace.a \
        $${DESTDIR}/liblibrepcbproject.a \
        $${DESTDIR}/liblibrepcblibrary.a \
        $${DESTDIR}/liblibrepcbcommon.a \
        $${DESTDIR}/libquazip.a \
        $${DESTDIR}/libpolyclipping.a \
}

RESOURCES += \
    ../../img/images.qrc \

SOURCES += \
    commandlineinterface.cpp \
    main.cpp \

HEADERS += \
    commandlineinterface.h \

# Hoedown is only needed for Qt <5.14
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 14) {
    LIBS += -lhoedown
    DEPENDPATH += ../../libs/hoedown
    PRE_TARGETDEPS += $${DESTDIR}/libhoedown.a
}

# QuaZIP
!contains(UNBUNDLE, quazip) {
    LIBS += -lquazip -lz
    INCLUDEPATH += ../../libs/quazip
    DEPENDPATH += ../../libs/quazip
}

# polyclipping
!contains(UNBUNDLE, polyclipping) {
    LIBS += -lpolyclipping
    DEPENDPATH += ../../libs/polyclipping
}
