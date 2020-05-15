#-------------------------------------------------
# Lib: Eagle importer
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbeagleimport

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

isEmpty(UNBUNDLE) {
    CONFIG += staticlib
} else {
    target.path = $${PREFIX}/lib
    INSTALLS += target
}

INCLUDEPATH += \
    ../../ \
    ../../parseagle \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \

RESOURCES += \
    ../../../img/images.qrc \

SOURCES += \
    converterdb.cpp \
    deviceconverter.cpp \
    devicesetconverter.cpp \
    packageconverter.cpp \
    symbolconverter.cpp \

HEADERS += \
    converterdb.h \
    deviceconverter.h \
    devicesetconverter.h \
    packageconverter.h \
    symbolconverter.h \

FORMS += \

