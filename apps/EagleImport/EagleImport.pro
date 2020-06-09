#-------------------------------------------------
# App: Eagle importer
#-------------------------------------------------

TEMPLATE = app
TARGET = eagle-import

# Use common project definitions
include(../../common.pri)

QT += core widgets xml network printsupport

# Note: The order of the libraries is very important for the linker!
# Another order could end up in "undefined reference" errors!
# Note that dependencies need to be listed *after* the dependent libs.
LIBS += \
    -L$${DESTDIR} \
    -llibrepcbeagleimport \
    -llibrepcblibrary \
    -llibrepcbcommon \
    -lparseagle \
    -lsexpresso \
    -lclipper \
    -lmuparser \

INCLUDEPATH += \
    ../../libs \
    ../../libs/parseagle \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/librepcb/eagleimport \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/parseagle \
    ../../libs/sexpresso \
    ../../libs/clipper \

isEmpty(UNBUNDLE) {
    # These libraries will only be linked statically when not unbundling
    PRE_TARGETDEPS += \
      $${DESTDIR}/liblibrepcbeagleimport.a \
      $${DESTDIR}/liblibrepcblibrary.a \
      $${DESTDIR}/liblibrepcbcommon.a \
}

PRE_TARGETDEPS += \
    $${DESTDIR}/libparseagle.a \
    $${DESTDIR}/libsexpresso.a \
    $${DESTDIR}/libclipper.a \

RESOURCES += \
    ../../img/images.qrc \

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    polygonsimplifier.cpp \

HEADERS += \
    mainwindow.h \
    polygonsimplifier.h \

FORMS += \
    mainwindow.ui \

# QuaZIP
!contains(UNBUNDLE, quazip) {
    LIBS += -lquazip -lz
    INCLUDEPATH += ../../libs/quazip
    DEPENDPATH += ../../libs/quazip
}
