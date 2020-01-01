#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibrarymanager

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport network

CONFIG += staticlib

INCLUDEPATH += \
    ../../ \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \

RESOURCES += \
    ../../../img/images.qrc \

SOURCES += \
    addlibrarywidget.cpp \
    librarydownload.cpp \
    libraryinfowidget.cpp \
    librarylistwidgetitem.cpp \
    librarymanager.cpp \
    repositorylibrarylistwidgetitem.cpp \

HEADERS += \
    addlibrarywidget.h \
    librarydownload.h \
    libraryinfowidget.h \
    librarylistwidgetitem.h \
    librarymanager.h \
    repositorylibrarylistwidgetitem.h \

FORMS += \
    addlibrarywidget.ui \
    libraryinfowidget.ui \
    librarylistwidgetitem.ui \
    librarymanager.ui \
    repositorylibrarylistwidgetitem.ui \

