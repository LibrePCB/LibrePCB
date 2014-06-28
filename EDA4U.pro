#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

QT += core widgets opengl webkitwidgets xml

TARGET = EDA4U
TEMPLATE = app

CONFIG += c++11

#DEFINES += USE_32BIT_LENGTH_UNITS          # see units.h

RC_ICONS = img/app.ico  # set the icon for windows
ICON = img/app.icns     # set the icon for mac

RESOURCES += \
    ressources.qrc

SOURCES += \
    src/main.cpp \
    src/common/units.cpp \
    src/common/cadscene.cpp \
    src/common/cadview.cpp \
    src/workspace/workspace.cpp \
    src/library/library.cpp \
    src/project/project.cpp \
    src/workspace/controlpanel/controlpanel.cpp \
    src/workspace/workspacechooserdialog.cpp \
    src/library_editor/libraryeditor.cpp \
    src/workspace/projecttreemodel.cpp \
    src/workspace/projecttreeitem.cpp \
    src/library/libraryelement.cpp \
    src/library/symbol.cpp \
    src/library/component.cpp \
    src/library/footprint.cpp \
    src/library/genericcomponent.cpp \
    src/library/model.cpp \
    src/library/package.cpp \
    src/library/spicemodel.cpp \
    src/library/componentcategory.cpp \
    src/library/packagecategory.cpp

HEADERS += \
    src/common/units.h \
    src/common/cadscene.h \
    src/common/cadview.h \
    src/workspace/workspace.h \
    src/library/library.h \
    src/project/project.h \
    src/workspace/controlpanel/controlpanel.h \
    src/workspace/workspacechooserdialog.h \
    src/library_editor/libraryeditor.h \
    src/workspace/projecttreemodel.h \
    src/workspace/projecttreeitem.h \
    src/library/libraryelement.h \
    src/library/symbol.h \
    src/library/component.h \
    src/library/footprint.h \
    src/library/genericcomponent.h \
    src/library/model.h \
    src/library/package.h \
    src/library/spicemodel.h \
    src/library/componentcategory.h \
    src/library/packagecategory.h

FORMS += \
    src/workspace/controlpanel/controlpanel.ui \
    src/workspace/workspacechooserdialog.ui \
    src/library_editor/libraryeditor.ui

