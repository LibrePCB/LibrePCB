#-------------------------------------------------
# Lib: Classes which represent a LibrePCB workspace
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbworkspace

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

isEmpty(UNBUNDLE) {
    CONFIG += staticlib
} else {
    target.path = $${LIBDIR}
    INSTALLS += target
}

INCLUDEPATH += \
    ../../ \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \

RESOURCES += \
    ../../../img/images.qrc \

SOURCES += \
    favoriteprojectsmodel.cpp \
    fileiconprovider.cpp \
    library/cat/categorytreeitem.cpp \
    library/cat/categorytreemodel.cpp \
    library/workspacelibrarydb.cpp \
    library/workspacelibraryscanner.cpp \
    projecttreemodel.cpp \
    recentprojectsmodel.cpp \
    settings/workspacesettings.cpp \
    settings/workspacesettingsdialog.cpp \
    settings/workspacesettingsitem.cpp \
    workspace.cpp \

HEADERS += \
    favoriteprojectsmodel.h \
    fileiconprovider.h \
    library/cat/categorytreeitem.h \
    library/cat/categorytreemodel.h \
    library/workspacelibrarydb.h \
    library/workspacelibraryscanner.h \
    projecttreemodel.h \
    recentprojectsmodel.h \
    settings/workspacesettings.h \
    settings/workspacesettingsdialog.h \
    settings/workspacesettingsitem.h \
    settings/workspacesettingsitem_genericvalue.h \
    settings/workspacesettingsitem_genericvaluelist.h \
    workspace.h \

FORMS += \
    settings/workspacesettingsdialog.ui \
    workspacechooserdialog.ui \

