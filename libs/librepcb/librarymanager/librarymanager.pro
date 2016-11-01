#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibrarymanager

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport network

CONFIG += staticlib

INCLUDEPATH += \
    ../../

SOURCES += \
    librarydownload.cpp \
    librarylistwidgetitem.cpp \
    libraryinfowidget.cpp \
    addlibrarywidget.cpp \
    librarymanager.cpp \
    repositorylibrarylistwidgetitem.cpp

HEADERS += \
    librarydownload.h \
    librarylistwidgetitem.h \
    libraryinfowidget.h \
    addlibrarywidget.h \
    librarymanager.h \
    repositorylibrarylistwidgetitem.h

FORMS += \
    librarylistwidgetitem.ui \
    libraryinfowidget.ui \
    addlibrarywidget.ui \
    librarymanager.ui \
    repositorylibrarylistwidgetitem.ui
