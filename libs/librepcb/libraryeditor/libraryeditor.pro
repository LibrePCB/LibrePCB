#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibraryeditor

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../

SOURCES += \
    cmpcat/componentcategoryeditorwidget.cpp \
    common/categorychooserdialog.cpp \
    common/categorytreelabeltextbuilder.cpp \
    common/editorwidgetbase.cpp \
    libraryeditor.cpp \
    pkgcat/packagecategoryeditorwidget.cpp \

HEADERS += \
    cmpcat/componentcategoryeditorwidget.h \
    common/categorychooserdialog.h \
    common/categorytreelabeltextbuilder.h \
    common/editorwidgetbase.h \
    libraryeditor.h \
    pkgcat/packagecategoryeditorwidget.h \

FORMS += \
    cmpcat/componentcategoryeditorwidget.ui \
    common/categorychooserdialog.ui \
    libraryeditor.ui \
    pkgcat/packagecategoryeditorwidget.ui \

