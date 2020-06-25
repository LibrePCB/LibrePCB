#-------------------------------------------------
# App: UUID generator
#-------------------------------------------------

TEMPLATE = app
TARGET = uuid-generator

# Use common project definitions
include(../../common.pri)

QT += core widgets

LIBS += \
    -L$${DESTDIR} \
    -llibrepcbcommon \
    -lsexpresso \
    -lclipper \
    -lmuparser \

# Solaris based systems need to link against libproc
solaris:LIBS += -lproc

INCLUDEPATH += \
    ../../libs \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/librepcb/common

isEmpty(UNBUNDLE) {
    # These libraries will only be linked statically when not unbundling
    PRE_TARGETDEPS += \
        $${DESTDIR}/liblibrepcbcommon.a
}

RESOURCES += \
    ../../img/images.qrc \

SOURCES += \
    main.cpp \
    mainwindow.cpp \

HEADERS += \
    mainwindow.h \

FORMS += \
    mainwindow.ui \

