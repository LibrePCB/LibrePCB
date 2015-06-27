#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbworkspace

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../

SOURCES += \
    workspace.cpp \
    projecttreemodel.cpp \
    projecttreeitem.cpp \
    recentprojectsmodel.cpp \
    favoriteprojectsmodel.cpp \
    settings/workspacesettings.cpp \
    settings/workspacesettingsdialog.cpp \
    settings/items/wsi_base.cpp \
    settings/items/wsi_applocale.cpp \
    settings/items/wsi_projectautosaveinterval.cpp \
    settings/items/wsi_librarylocaleorder.cpp \
    settings/items/wsi_appdefaultmeasurementunits.cpp \
    settings/items/wsi_librarynormorder.cpp \
    settings/items/wsi_debugtools.cpp \
    settings/items/wsi_appearance.cpp

HEADERS += \
    workspace.h \
    projecttreemodel.h \
    projecttreeitem.h \
    recentprojectsmodel.h \
    favoriteprojectsmodel.h \
    settings/workspacesettings.h \
    settings/workspacesettingsdialog.h \
    settings/items/wsi_base.h \
    settings/items/wsi_applocale.h \
    settings/items/wsi_projectautosaveinterval.h \
    settings/items/wsi_librarylocaleorder.h \
    settings/items/wsi_appdefaultmeasurementunits.h \
    settings/items/wsi_librarynormorder.h \
    settings/items/wsi_debugtools.h \
    settings/items/wsi_appearance.h

FORMS += \
    workspacechooserdialog.ui \
    settings/workspacesettingsdialog.ui
