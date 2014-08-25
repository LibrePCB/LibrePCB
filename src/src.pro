#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

TEMPLATE = app
TARGET = eda4u

# Set the path for the generated binary
GENERATED_DIR = ../generated

# Use common project definitions
include(../common.pri)

QT += core widgets opengl webkitwidgets xml

exists(../.git):DEFINES += GIT_BRANCH=\\\"master\\\"
#DEFINES += USE_32BIT_LENGTH_UNITS          # see units.h

win32 {
    # Windows-specific configurations
    RC_ICONS = ../packaging/windows/img/eda4u.ico
}

macx {
    # Mac-specific configurations
    ICON = ../packaging/mac/img/eda4u.icns
}

unix:!macx {
    # Linux/UNIX-specific configurations
    target.path = /usr/local/bin
    icon.path = /usr/share/pixmaps
    icon.files = ../packaging/unix/img/eda4u.xpm
    desktop.path = /usr/share/applications
    desktop.files = ../packaging/unix/eda4u.desktop
    mimexml.path = /usr/share/mime/packages
    mimexml.files = ../packaging/unix/mime/eda4u.xml
    mimedesktop.path = /usr/share/mimelnk/application
    mimedesktop.files = ../packaging/unix/mime/x-eda4u-project.desktop
    INSTALLS += target icon desktop mimexml mimedesktop
}

TRANSLATIONS = \
    ../i18n/eda4u_de.ts \
    ../i18n/eda4u_de_CH.ts \
    ../i18n/eda4u_gsw_CH.ts

RESOURCES += \
    ../img/images.qrc \
    ../i18n/translations.qrc

SOURCES += \
    main.cpp \
    common/units.cpp \
    common/cadscene.cpp \
    common/cadview.cpp \
    workspace/workspace.cpp \
    library/library.cpp \
    project/project.cpp \
    workspace/controlpanel/controlpanel.cpp \
    library_editor/libraryeditor.cpp \
    workspace/projecttreemodel.cpp \
    workspace/projecttreeitem.cpp \
    library/libraryelement.cpp \
    library/symbol.cpp \
    library/component.cpp \
    library/footprint.cpp \
    library/genericcomponent.cpp \
    library/model.cpp \
    library/package.cpp \
    library/spicemodel.cpp \
    library/componentcategory.cpp \
    library/packagecategory.cpp \
    project/schematics/schematiceditor.cpp \
    project/circuit/circuit.cpp \
    workspace/recentprojectsmodel.cpp \
    workspace/favoriteprojectsmodel.cpp \
    common/exceptions.cpp \
    workspace/workspacesettings.cpp \
    workspace/workspacesettingsdialog.cpp \
    project/schematics/schematic.cpp \
    project/boards/board.cpp \
    project/library/projectlibrary.cpp \
    project/circuit/netclass.cpp \
    project/circuit/netsignal.cpp \
    project/circuit/genericcomponentinstance.cpp \
    common/systeminfo.cpp \
    common/debug.cpp \
    common/filelock.cpp \
    common/filepath.cpp \
    common/schematiclayer.cpp \
    common/xmlfile.cpp \
    library/librarybaseelement.cpp \
    project/circuit/cmd/cmdnetclassadd.cpp \
    project/circuit/cmd/cmdnetclassremove.cpp \
    project/circuit/cmd/cmdnetsignaladd.cpp \
    project/circuit/cmd/cmdnetsignalremove.cpp \
    common/undocommand.cpp \
    common/undostack.cpp \
    project/schematics/symbolinstance.cpp \
    project/schematics/schematicpagesdock.cpp \
    common/inifile.cpp \
    project/schematics/cmd/cmdschematicadd.cpp \
    project/schematics/cmd/cmdschematicremove.cpp \
    project/schematics/unplacedsymbolsdock.cpp

HEADERS += \
    common/units.h \
    common/cadscene.h \
    common/cadview.h \
    workspace/workspace.h \
    library/library.h \
    project/project.h \
    workspace/controlpanel/controlpanel.h \
    library_editor/libraryeditor.h \
    workspace/projecttreemodel.h \
    workspace/projecttreeitem.h \
    library/libraryelement.h \
    library/symbol.h \
    library/component.h \
    library/footprint.h \
    library/genericcomponent.h \
    library/model.h \
    library/package.h \
    library/spicemodel.h \
    library/componentcategory.h \
    library/packagecategory.h \
    project/schematics/schematiceditor.h \
    project/circuit/circuit.h \
    workspace/recentprojectsmodel.h \
    workspace/favoriteprojectsmodel.h \
    common/exceptions.h \
    workspace/workspacesettings.h \
    workspace/workspacesettingsdialog.h \
    project/schematics/schematic.h \
    project/boards/board.h \
    project/library/projectlibrary.h \
    project/circuit/netclass.h \
    project/circuit/netsignal.h \
    project/circuit/genericcomponentinstance.h \
    common/systeminfo.h \
    common/debug.h \
    common/filelock.h \
    common/filepath.h \
    common/schematiclayer.h \
    common/xmlfile.h \
    library/librarybaseelement.h \
    project/circuit/cmd/cmdnetclassadd.h \
    project/circuit/cmd/cmdnetclassremove.h \
    project/circuit/cmd/cmdnetsignaladd.h \
    project/circuit/cmd/cmdnetsignalremove.h \
    common/undocommand.h \
    common/undostack.h \
    project/schematics/symbolinstance.h \
    project/schematics/schematicpagesdock.h \
    common/inifile.h \
    project/schematics/cmd/cmdschematicadd.h \
    project/schematics/cmd/cmdschematicremove.h \
    project/schematics/unplacedsymbolsdock.h

FORMS += \
    workspace/controlpanel/controlpanel.ui \
    workspace/workspacechooserdialog.ui \
    library_editor/libraryeditor.ui \
    project/schematics/schematiceditor.ui \
    workspace/workspacesettingsdialog.ui \
    project/schematics/schematicpagesdock.ui \
    project/schematics/unplacedsymbolsdock.ui


# Custom compiler "lrelease" for qm generation
isEmpty(QMAKE_LRELEASE) {
    win32: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    else: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
lrelease.input = TRANSLATIONS
lrelease.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
lrelease.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
lrelease.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += lrelease
PRE_TARGETDEPS += compiler_lrelease_make_all
