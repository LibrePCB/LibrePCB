#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbeagleimport

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../ \
    ../../parseagle \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \

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

