#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

QT += core widgets opengl webkitwidgets xml

TARGET = eda4u
TEMPLATE = app

CONFIG += c++11

exists(.git):DEFINES += GIT_BRANCH=\\\"master\\\"
#DEFINES += USE_32BIT_LENGTH_UNITS          # see units.h

win32 {
    # Windows-specific configurations
    RC_ICONS = packaging/windows/img/eda4u.ico
}

macx {
    # Mac-specific configurations
    ICON = packaging/mac/img/eda4u.icns
}

unix:!macx {
    # Linux/UNIX-specific configurations
    target.path = /usr/local/bin
    icon.path = /usr/share/pixmaps
    icon.files = packaging/unix/img/eda4u.xpm
    desktop.path = /usr/share/applications
    desktop.files = packaging/unix/eda4u.desktop
    mimexml.path = /usr/share/mime/packages
    mimexml.files = packaging/unix/mime/eda4u.xml
    mimedesktop.path = /usr/share/mimelnk/application
    mimedesktop.files = packaging/unix/mime/x-eda4u-project.desktop
    INSTALLS += target icon desktop mimexml mimedesktop
}

TRANSLATIONS = i18n/eda4u_en.ts \
    i18n/eda4u_de.ts \
    i18n/eda4u_de_CH.ts

RESOURCES += \
    ressources.qrc \
    img/images.qrc \
    i18n/translations.qrc

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
    src/library/packagecategory.cpp \
    src/common/layer.cpp \
    src/project/schematics/schematiceditor.cpp \
    src/project/circuit/circuit.cpp \
    src/workspace/recentprojectsmodel.cpp

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
    src/library/packagecategory.h \
    src/common/layer.h \
    src/project/schematics/schematiceditor.h \
    src/project/circuit/circuit.h \
    src/workspace/recentprojectsmodel.h \
    src/common/exceptions.h

FORMS += \
    src/workspace/controlpanel/controlpanel.ui \
    src/workspace/workspacechooserdialog.ui \
    src/library_editor/libraryeditor.ui \
    src/project/schematics/schematiceditor.ui

