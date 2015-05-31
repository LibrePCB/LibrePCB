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

QT += core widgets opengl webkitwidgets xml printsupport sql

# Define the application version here
DEFINES += APP_VERSION_MAJOR=0
DEFINES += APP_VERSION_MINOR=1

exists(../.git):DEFINES += GIT_BRANCH=\\\"master\\\"

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

LIBS += \
    -L$${DESTDIR} \
    -leda4ucommon \
    -leda4ulibrary

INCLUDEPATH += \
    ../lib

DEPENDPATH += \
    ../lib/eda4ucommon \
    ../lib/eda4ulibrary

PRE_TARGETDEPS += \
    $${DESTDIR}/libeda4ucommon.a \
    $${DESTDIR}/libeda4ulibrary.a

TRANSLATIONS = \
    ../i18n/eda4u_de.ts \
    ../i18n/eda4u_de_CH.ts \
    ../i18n/eda4u_gsw_CH.ts

RESOURCES += \
    ../img/images.qrc \
    ../i18n/translations.qrc

SOURCES += \
    main.cpp \
    workspace/workspace.cpp \
    library/library.cpp \
    project/project.cpp \
    workspace/controlpanel/controlpanel.cpp \
    library_editor/libraryeditor.cpp \
    workspace/projecttreemodel.cpp \
    workspace/projecttreeitem.cpp \
    library/libraryelement.cpp \
    library/sym/symbol.cpp \
    library/cmp/component.cpp \
    library/fpt/footprint.cpp \
    library/gencmp/genericcomponent.cpp \
    library/3dmdl/model3d.cpp \
    library/pkg/package.cpp \
    library/spcmdl/spicemodel.cpp \
    library/cat/componentcategory.cpp \
    library/cat/packagecategory.cpp \
    project/schematics/schematiceditor.cpp \
    project/circuit/circuit.cpp \
    workspace/recentprojectsmodel.cpp \
    workspace/favoriteprojectsmodel.cpp \
    workspace/settings/workspacesettings.cpp \
    workspace/settings/workspacesettingsdialog.cpp \
    project/schematics/schematic.cpp \
    project/boards/board.cpp \
    project/library/projectlibrary.cpp \
    project/circuit/netclass.cpp \
    project/circuit/netsignal.cpp \
    project/circuit/gencompsignalinstance.cpp \
    library/librarybaseelement.cpp \
    project/circuit/cmd/cmdnetclassadd.cpp \
    project/circuit/cmd/cmdnetclassremove.cpp \
    project/circuit/cmd/cmdnetsignaladd.cpp \
    project/circuit/cmd/cmdnetsignalremove.cpp \
    project/schematics/schematicpagesdock.cpp \
    project/schematics/cmd/cmdschematicadd.cpp \
    project/schematics/cmd/cmdschematicremove.cpp \
    project/erc/ercmsgdock.cpp \
    project/schematics/fsm/schematiceditorevent.cpp \
    project/schematics/fsm/ses_select.cpp \
    project/schematics/fsm/ses_move.cpp \
    project/schematics/fsm/ses_drawtext.cpp \
    project/schematics/fsm/ses_drawrect.cpp \
    project/schematics/fsm/ses_drawpolygon.cpp \
    project/schematics/fsm/ses_drawcircle.cpp \
    project/schematics/fsm/ses_drawellipse.cpp \
    project/schematics/fsm/ses_drawwire.cpp \
    project/schematics/fsm/ses_addcomponents.cpp \
    project/schematics/cmd/cmdschematicnetpointadd.cpp \
    project/schematics/cmd/cmdschematicnetpointremove.cpp \
    project/schematics/cmd/cmdschematicnetlineadd.cpp \
    project/schematics/cmd/cmdschematicnetlineremove.cpp \
    workspace/settings/items/wsi_base.cpp \
    workspace/settings/items/wsi_applocale.cpp \
    workspace/settings/items/wsi_projectautosaveinterval.cpp \
    workspace/settings/items/wsi_librarylocaleorder.cpp \
    project/circuit/editnetclassesdialog.cpp \
    workspace/settings/items/wsi_appdefaultmeasurementunits.cpp \
    library/gencmp/gencompsignal.cpp \
    library/gencmp/gencompsymbvar.cpp \
    library/gencmp/gencompsymbvaritem.cpp \
    library/sym/symbolpin.cpp \
    library/sym/symbolpolygon.cpp \
    library/sym/symboltext.cpp \
    workspace/settings/items/wsi_librarynormorder.cpp \
    project/schematics/cmd/cmdsymbolinstanceadd.cpp \
    project/circuit/cmd/cmdgencompsiginstsetnetsignal.cpp \
    project/schematics/fsm/ses_base.cpp \
    project/schematics/fsm/ses_fsm.cpp \
    project/schematics/cmd/cmdsymbolinstanceremove.cpp \
    project/schematics/cmd/cmdschematicnetpointdetach.cpp \
    workspace/settings/items/wsi_debugtools.cpp \
    project/erc/ercmsg.cpp \
    project/erc/ercmsglist.cpp \
    project/circuit/gencompinstance.cpp \
    library/libraryelementattribute.cpp \
    project/circuit/gencompattributeinstance.cpp \
    project/schematics/symbolinstancepropertiesdialog.cpp \
    project/dialogs/projectpropertieseditordialog.cpp \
    project/cmd/cmdprojectsetmetadata.cpp \
    workspace/settings/items/wsi_appearance.cpp \
    project/dialogs/addgencompdialog.cpp \
    project/schematics/fsm/ses_addnetlabel.cpp \
    project/schematics/cmd/cmdschematicnetlabeladd.cpp \
    project/schematics/cmd/cmdschematicnetlabelremove.cpp \
    project/schematics/cmd/cmdschematicnetlabeledit.cpp \
    project/schematics/schematicclipboard.cpp \
    library/sym/symbolellipse.cpp \
    project/circuit/cmd/cmdgencompinstedit.cpp \
    project/circuit/cmd/cmdnetclassedit.cpp \
    project/circuit/cmd/cmdnetsignaledit.cpp \
    project/schematics/cmd/cmdschematicnetpointedit.cpp \
    project/schematics/cmd/cmdsymbolinstanceedit.cpp \
    project/circuit/cmd/cmdgencompinstadd.cpp \
    project/circuit/cmd/cmdgencompinstremove.cpp \
    project/circuit/cmd/cmdgencompattrinstadd.cpp \
    project/circuit/cmd/cmdgencompattrinstremove.cpp \
    project/circuit/cmd/cmdgencompattrinstedit.cpp \
    project/settings/projectsettings.cpp \
    project/settings/projectsettingsdialog.cpp \
    project/settings/cmd/cmdprojectsettingschange.cpp \
    project/schematics/items/si_base.cpp \
    project/schematics/items/si_netlabel.cpp \
    project/schematics/items/si_netline.cpp \
    project/schematics/items/si_netpoint.cpp \
    project/schematics/items/si_symbol.cpp \
    project/schematics/items/si_symbolpin.cpp \
    project/schematics/graphicsitems/sgi_base.cpp \
    project/schematics/graphicsitems/sgi_netlabel.cpp \
    project/schematics/graphicsitems/sgi_netline.cpp \
    project/schematics/graphicsitems/sgi_netpoint.cpp \
    project/schematics/graphicsitems/sgi_symbol.cpp \
    project/schematics/graphicsitems/sgi_symbolpin.cpp \
    library/sym/symbolpreviewgraphicsitem.cpp \
    library/sym/symbolpinpreviewgraphicsitem.cpp \
    project/boards/boardeditor.cpp \
    project/boards/cmd/cmdboardadd.cpp \
    project/boards/items/bi_base.cpp \
    project/boards/componentinstance.cpp \
    project/boards/items/bi_footprint.cpp \
    project/boards/graphicsitems/bgi_base.cpp \
    project/boards/graphicsitems/bgi_footprint.cpp

HEADERS += \
    workspace/workspace.h \
    library/library.h \
    project/project.h \
    workspace/controlpanel/controlpanel.h \
    library_editor/libraryeditor.h \
    workspace/projecttreemodel.h \
    workspace/projecttreeitem.h \
    library/libraryelement.h \
    library/sym/symbol.h \
    library/cmp/component.h \
    library/fpt/footprint.h \
    library/gencmp/genericcomponent.h \
    library/3dmdl/model3d.h \
    library/pkg/package.h \
    library/spcmdl/spicemodel.h \
    library/cat/componentcategory.h \
    library/cat/packagecategory.h \
    project/schematics/schematiceditor.h \
    project/circuit/circuit.h \
    workspace/recentprojectsmodel.h \
    workspace/favoriteprojectsmodel.h \
    workspace/settings/workspacesettings.h \
    workspace/settings/workspacesettingsdialog.h \
    project/schematics/schematic.h \
    project/boards/board.h \
    project/library/projectlibrary.h \
    project/circuit/netclass.h \
    project/circuit/netsignal.h \
    project/circuit/gencompsignalinstance.h \
    library/librarybaseelement.h \
    project/circuit/cmd/cmdnetclassadd.h \
    project/circuit/cmd/cmdnetclassremove.h \
    project/circuit/cmd/cmdnetsignaladd.h \
    project/circuit/cmd/cmdnetsignalremove.h \
    project/schematics/schematicpagesdock.h \
    project/schematics/cmd/cmdschematicadd.h \
    project/schematics/cmd/cmdschematicremove.h \
    project/erc/ercmsgdock.h \
    project/schematics/fsm/schematiceditorevent.h \
    project/schematics/fsm/ses_select.h \
    project/schematics/fsm/ses_move.h \
    project/schematics/fsm/ses_drawtext.h \
    project/schematics/fsm/ses_drawrect.h \
    project/schematics/fsm/ses_drawpolygon.h \
    project/schematics/fsm/ses_drawcircle.h \
    project/schematics/fsm/ses_drawellipse.h \
    project/schematics/fsm/ses_drawwire.h \
    project/schematics/fsm/ses_addcomponents.h \
    project/schematics/cmd/cmdschematicnetpointadd.h \
    project/schematics/cmd/cmdschematicnetpointremove.h \
    project/schematics/cmd/cmdschematicnetlineadd.h \
    project/schematics/cmd/cmdschematicnetlineremove.h \
    workspace/settings/items/wsi_base.h \
    workspace/settings/items/wsi_applocale.h \
    workspace/settings/items/wsi_projectautosaveinterval.h \
    workspace/settings/items/wsi_librarylocaleorder.h \
    project/circuit/editnetclassesdialog.h \
    workspace/settings/items/wsi_appdefaultmeasurementunits.h \
    library/gencmp/gencompsignal.h \
    library/gencmp/gencompsymbvar.h \
    library/gencmp/gencompsymbvaritem.h \
    library/sym/symbolpin.h \
    library/sym/symbolpolygon.h \
    library/sym/symboltext.h \
    workspace/settings/items/wsi_librarynormorder.h \
    project/schematics/cmd/cmdsymbolinstanceadd.h \
    project/circuit/cmd/cmdgencompsiginstsetnetsignal.h \
    project/schematics/fsm/ses_base.h \
    project/schematics/fsm/ses_fsm.h \
    project/schematics/cmd/cmdsymbolinstanceremove.h \
    project/schematics/cmd/cmdschematicnetpointdetach.h \
    workspace/settings/items/wsi_debugtools.h \
    project/erc/ercmsg.h \
    project/erc/ercmsglist.h \
    project/circuit/gencompinstance.h \
    library/libraryelementattribute.h \
    project/circuit/gencompattributeinstance.h \
    project/schematics/symbolinstancepropertiesdialog.h \
    project/dialogs/projectpropertieseditordialog.h \
    project/cmd/cmdprojectsetmetadata.h \
    project/erc/if_ercmsgprovider.h \
    workspace/settings/items/wsi_appearance.h \
    project/dialogs/addgencompdialog.h \
    project/schematics/fsm/ses_addnetlabel.h \
    project/schematics/cmd/cmdschematicnetlabeladd.h \
    project/schematics/cmd/cmdschematicnetlabelremove.h \
    project/schematics/cmd/cmdschematicnetlabeledit.h \
    project/schematics/schematicclipboard.h \
    library/sym/symbolellipse.h \
    project/circuit/cmd/cmdgencompinstedit.h \
    project/circuit/cmd/cmdnetclassedit.h \
    project/circuit/cmd/cmdnetsignaledit.h \
    project/schematics/cmd/cmdschematicnetpointedit.h \
    project/schematics/cmd/cmdsymbolinstanceedit.h \
    project/circuit/cmd/cmdgencompinstadd.h \
    project/circuit/cmd/cmdgencompinstremove.h \
    project/circuit/cmd/cmdgencompattrinstadd.h \
    project/circuit/cmd/cmdgencompattrinstremove.h \
    project/circuit/cmd/cmdgencompattrinstedit.h \
    project/settings/projectsettings.h \
    project/settings/projectsettingsdialog.h \
    project/settings/cmd/cmdprojectsettingschange.h \
    project/schematics/items/si_base.h \
    project/schematics/items/si_netlabel.h \
    project/schematics/items/si_netline.h \
    project/schematics/items/si_netpoint.h \
    project/schematics/items/si_symbol.h \
    project/schematics/items/si_symbolpin.h \
    project/schematics/graphicsitems/sgi_base.h \
    project/schematics/graphicsitems/sgi_netlabel.h \
    project/schematics/graphicsitems/sgi_netline.h \
    project/schematics/graphicsitems/sgi_netpoint.h \
    project/schematics/graphicsitems/sgi_symbol.h \
    project/schematics/graphicsitems/sgi_symbolpin.h \
    library/sym/symbolpreviewgraphicsitem.h \
    library/sym/symbolpinpreviewgraphicsitem.h \
    project/boards/boardeditor.h \
    project/boards/cmd/cmdboardadd.h \
    project/boards/items/bi_base.h \
    project/boards/componentinstance.h \
    project/boards/items/bi_footprint.h \
    project/boards/graphicsitems/bgi_base.h \
    project/boards/graphicsitems/bgi_footprint.h

FORMS += \
    workspace/controlpanel/controlpanel.ui \
    workspace/workspacechooserdialog.ui \
    library_editor/libraryeditor.ui \
    project/schematics/schematiceditor.ui \
    workspace/settings/workspacesettingsdialog.ui \
    project/schematics/schematicpagesdock.ui \
    project/erc/ercmsgdock.ui \
    project/circuit/editnetclassesdialog.ui \
    project/schematics/symbolinstancepropertiesdialog.ui \
    project/dialogs/projectpropertieseditordialog.ui \
    project/dialogs/addgencompdialog.ui \
    project/settings/projectsettingsdialog.ui \
    project/boards/boardeditor.ui


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
