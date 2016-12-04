#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbworkspace

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../

SOURCES += \
    favoriteprojectsmodel.cpp \
    library/cat/categorytreeitem.cpp \
    library/cat/categorytreemodel.cpp \
    library/workspacelibrarydb.cpp \
    library/workspacelibraryscanner.cpp \
    projecttreeitem.cpp \
    projecttreemodel.cpp \
    recentprojectsmodel.cpp \
    settings/items/wsi_appdefaultmeasurementunits.cpp \
    settings/items/wsi_appearance.cpp \
    settings/items/wsi_applocale.cpp \
    settings/items/wsi_base.cpp \
    settings/items/wsi_debugtools.cpp \
    settings/items/wsi_librarylocaleorder.cpp \
    settings/items/wsi_librarynormorder.cpp \
    settings/items/wsi_projectautosaveinterval.cpp \
    settings/items/wsi_repositories.cpp \
    settings/workspacesettings.cpp \
    settings/workspacesettingsdialog.cpp \
    workspace.cpp \

HEADERS += \
    favoriteprojectsmodel.h \
    library/cat/categorytreeitem.h \
    library/cat/categorytreemodel.h \
    library/workspacelibrarydb.h \
    library/workspacelibraryscanner.h \
    projecttreeitem.h \
    projecttreemodel.h \
    recentprojectsmodel.h \
    settings/items/wsi_appdefaultmeasurementunits.h \
    settings/items/wsi_appearance.h \
    settings/items/wsi_applocale.h \
    settings/items/wsi_base.h \
    settings/items/wsi_debugtools.h \
    settings/items/wsi_librarylocaleorder.h \
    settings/items/wsi_librarynormorder.h \
    settings/items/wsi_projectautosaveinterval.h \
    settings/items/wsi_repositories.h \
    settings/workspacesettings.h \
    settings/workspacesettingsdialog.h \
    workspace.h \

FORMS += \
    settings/workspacesettingsdialog.ui \
    workspacechooserdialog.ui \

