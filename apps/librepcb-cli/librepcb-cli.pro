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
    -lclipper \
    -lhoedown \
    -lmuparser \
    -lsexpresso \

INCLUDEPATH += \
    ../../libs \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/hoedown \
    ../../libs/librepcb/librarymanager \
    ../../libs/librepcb/projecteditor \
    ../../libs/librepcb/libraryeditor \
    ../../libs/librepcb/workspace \
    ../../libs/librepcb/project \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/sexpresso \
    ../../libs/clipper \
    ../../libs/muparser \

PRE_TARGETDEPS += \
    $${DESTDIR}/libhoedown.a \
    $${DESTDIR}/libsexpresso.a \
    $${DESTDIR}/libclipper.a \
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
}

RESOURCES += \
    ../../img/images.qrc \

SOURCES += \
    commandlineinterface.cpp \
    main.cpp \

HEADERS += \
    commandlineinterface.h \

# QuaZIP
!contains(UNBUNDLE, quazip) {
    LIBS += -lquazip -lz
    INCLUDEPATH += ../../libs/quazip
    DEPENDPATH += ../../libs/quazip
}
