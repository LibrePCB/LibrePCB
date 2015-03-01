#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

TEMPLATE = app
TARGET = eagle-import

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

# Path to UUID_List.ini
isEmpty(UUID_LIST_FILEPATH):UUID_LIST_FILEPATH = $$absolute_path("UUID_List.ini")
DEFINES += UUID_LIST_FILEPATH=\\\"$${UUID_LIST_FILEPATH}\\\"

# Define the application version here (needed for XML files)
DEFINES += APP_VERSION_MAJOR=0
DEFINES += APP_VERSION_MINOR=1

QT += core widgets opengl webkitwidgets xml printsupport sql

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    workspace.cpp

HEADERS += \
    mainwindow.h \
    workspace.h

FORMS += mainwindow.ui

#-------------------------------------------------
# Include parts of the main project
#-------------------------------------------------

INCLUDEPATH += ../../src

SOURCES += \
    ../../src/common/units/length.cpp \
    ../../src/common/units/lengthunit.cpp \
    ../../src/common/units/angle.cpp \
    ../../src/common/units/point.cpp \
#    ../../src/common/cadscene.cpp \
#    ../../src/common/cadview.cpp \
#    ../../src/workspace/workspace.cpp \
#    ../../src/library/library.cpp \
#    ../../src/project/project.cpp \
#    ../../src/workspace/controlpanel/controlpanel.cpp \
#    ../../src/library_editor/libraryeditor.cpp \
#    ../../src/workspace/projecttreemodel.cpp \
#    ../../src/workspace/projecttreeitem.cpp \
    ../../src/library/libraryelement.cpp \
    ../../src/library/symbol.cpp \
#    ../../src/library/component.cpp \
#    ../../src/library/footprint.cpp \
    ../../src/library/genericcomponent.cpp \
#    ../../src/library/model.cpp \
#    ../../src/library/package.cpp \
#    ../../src/library/spicemodel.cpp \
#    ../../src/library/componentcategory.cpp \
#    ../../src/library/packagecategory.cpp \
#    ../../src/project/schematics/schematiceditor.cpp \
#    ../../src/project/circuit/circuit.cpp \
#    ../../src/workspace/recentprojectsmodel.cpp \
#    ../../src/workspace/favoriteprojectsmodel.cpp \
    ../../src/common/exceptions.cpp \
#    ../../src/workspace/settings/workspacesettings.cpp \
#    ../../src/workspace/settings/workspacesettingsdialog.cpp \
#    ../../src/project/schematics/schematic.cpp \
#    ../../src/project/boards/board.cpp \
#    ../../src/project/library/projectlibrary.cpp \
#    ../../src/project/circuit/netclass.cpp \
#    ../../src/project/circuit/netsignal.cpp \
#    ../../src/project/circuit/gencompsignalinstance.cpp \
#    ../../src/common/systeminfo.cpp \
    ../../src/common/debug.cpp \
#    ../../src/common/filelock.cpp \
    ../../src/common/filepath.cpp \
#    ../../src/common/schematiclayer.cpp \
    ../../src/common/smartxmlfile.cpp \
    ../../src/library/librarybaseelement.cpp \
#    ../../src/project/circuit/cmd/cmdnetclassadd.cpp \
#    ../../src/project/circuit/cmd/cmdnetclassremove.cpp \
#    ../../src/project/circuit/cmd/cmdnetsignaladd.cpp \
#    ../../src/project/circuit/cmd/cmdnetsignalremove.cpp \
#    ../../src/common/undocommand.cpp \
#    ../../src/common/undostack.cpp \
#    ../../src/project/schematics/symbolinstance.cpp \
#    ../../src/project/schematics/schematicpagesdock.cpp \
#    ../../src/common/smartinifile.cpp \
#    ../../src/project/schematics/cmd/cmdschematicadd.cpp \
#    ../../src/project/schematics/cmd/cmdschematicremove.cpp \
#    ../../src/project/erc/ercmsgdock.cpp \
#    ../../src/project/schematics/fsm/schematiceditorevent.cpp \
#    ../../src/project/schematics/fsm/ses_select.cpp \
#    ../../src/project/schematics/fsm/ses_move.cpp \
#    ../../src/project/schematics/fsm/ses_drawtext.cpp \
#    ../../src/project/schematics/fsm/ses_drawrect.cpp \
#    ../../src/project/schematics/fsm/ses_drawpolygon.cpp \
#    ../../src/project/schematics/fsm/ses_drawcircle.cpp \
#    ../../src/project/schematics/fsm/ses_drawellipse.cpp \
#    ../../src/project/schematics/fsm/ses_drawwire.cpp \
#    ../../src/project/schematics/fsm/ses_addcomponents.cpp \
#    ../../src/project/schematics/schematicnetpoint.cpp \
#    ../../src/project/schematics/schematicnetline.cpp \
#    ../../src/project/schematics/cmd/cmdschematicnetpointadd.cpp \
#    ../../src/project/schematics/cmd/cmdschematicnetpointremove.cpp \
#    ../../src/project/schematics/cmd/cmdschematicnetlineadd.cpp \
#    ../../src/project/schematics/cmd/cmdschematicnetlineremove.cpp \
#    ../../src/workspace/settings/items/wsi_base.cpp \
#    ../../src/workspace/settings/items/wsi_applocale.cpp \
#    ../../src/workspace/settings/items/wsi_projectautosaveinterval.cpp \
#    ../../src/workspace/settings/items/wsi_librarylocaleorder.cpp \
#    ../../src/project/circuit/editnetclassesdialog.cpp \
#    ../../src/project/circuit/cmd/cmdnetclasssetname.cpp \
#    ../../src/common/dialogs/gridsettingsdialog.cpp \
#    ../../src/workspace/settings/items/wsi_appdefaultmeasurementunits.cpp \
#    ../../src/common/application.cpp \
    ../../src/library/gencompsignal.cpp \
    ../../src/library/gencompsymbvar.cpp \
    ../../src/library/gencompsymbvaritem.cpp \
    ../../src/library/symbolpin.cpp \
    ../../src/library/symbolpolygon.cpp \
    ../../src/library/symboltext.cpp \
#    ../../src/library/symbolgraphicsitem.cpp \
#    ../../src/project/schematics/symbolpininstance.cpp \
#    ../../src/library/symbolpingraphicsitem.cpp \
    ../../src/common/version.cpp \
#    ../../src/workspace/settings/items/wsi_librarynormorder.cpp \
#    ../../src/project/schematics/cmd/cmdsymbolinstancemove.cpp \
#    ../../src/project/circuit/cmd/cmdgencompinstanceadd.cpp \
#    ../../src/project/schematics/cmd/cmdsymbolinstanceadd.cpp \
#    ../../src/project/schematics/cmd/cmdschematicnetpointmove.cpp \
#    ../../src/project/circuit/cmd/cmdgencompsiginstsetnetsignal.cpp \
#    ../../src/project/schematics/fsm/ses_base.cpp \
#    ../../src/project/schematics/fsm/ses_fsm.cpp \
#    ../../src/project/schematics/cmd/cmdsymbolinstanceremove.cpp \
#    ../../src/project/schematics/cmd/cmdschematicnetpointdetach.cpp \
#    ../../src/project/schematics/cmd/cmdschematicnetpointsetnetsignal.cpp \
#    ../../src/workspace/settings/items/wsi_debugtools.cpp \
#    ../../src/project/erc/ercmsg.cpp \
#    ../../src/project/erc/ercmsglist.cpp \
#    ../../src/project/circuit/gencompinstance.cpp \
    ../../src/library/attribute.cpp \
#    ../../src/project/circuit/gencompattributeinstance.cpp \
#    ../../src/common/if_attributeprovider.cpp \
#    ../../src/project/schematics/symbolinstancepropertiesdialog.cpp \
#    ../../src/project/circuit/cmd/cmdgencompinstsetname.cpp \
#    ../../src/project/circuit/cmd/cmdgencompinstsetvalue.cpp \
#    ../../src/project/circuit/cmd/cmdnetsignalsetname.cpp \
#    ../../src/project/dialogs/projectpropertieseditordialog.cpp \
#    ../../src/project/cmd/cmdprojectsetmetadata.cpp \
#    ../../src/common/smarttextfile.cpp \
    ../../src/common/smartfile.cpp \
    ../../src/common/file_io/xmldomdocument.cpp \
    ../../src/common/file_io/xmldomelement.cpp \
#    ../../src/workspace/settings/items/wsi_appearance.cpp \
#    ../../src/project/dialogs/addgencompdialog.cpp \
    ../../src/common/alignment.cpp

HEADERS += \
    ../../src/common/units/all_length_units.h \
    ../../src/common/units/length.h \
    ../../src/common/units/lengthunit.h \
    ../../src/common/units/angle.h \
    ../../src/common/units/point.h \
#    ../../src/common/cadscene.h \
#    ../../src/common/cadview.h \
#    ../../src/workspace/workspace.h \
#    ../../src/library/library.h \
#    ../../src/project/project.h \
#    ../../src/workspace/controlpanel/controlpanel.h \
#    ../../src/library_editor/libraryeditor.h \
#    ../../src/workspace/projecttreemodel.h \
#    ../../src/workspace/projecttreeitem.h \
    ../../src/library/libraryelement.h \
    ../../src/library/symbol.h \
#    ../../src/library/component.h \
#    ../../src/library/footprint.h \
    ../../src/library/genericcomponent.h \
#    ../../src/library/model.h \
#    ../../src/library/package.h \
#    ../../src/library/spicemodel.h \
#    ../../src/library/componentcategory.h \
#    ../../src/library/packagecategory.h \
#    ../../src/project/schematics/schematiceditor.h \
#    ../../src/project/circuit/circuit.h \
#    ../../src/workspace/recentprojectsmodel.h \
#    ../../src/workspace/favoriteprojectsmodel.h \
    ../../src/common/exceptions.h \
#    ../../src/workspace/settings/workspacesettings.h \
#    ../../src/workspace/settings/workspacesettingsdialog.h \
#    ../../src/project/schematics/schematic.h \
#    ../../src/project/boards/board.h \
#    ../../src/project/library/projectlibrary.h \
#    ../../src/project/circuit/netclass.h \
#    ../../src/project/circuit/netsignal.h \
#    ../../src/project/circuit/gencompsignalinstance.h \
#    ../../src/common/systeminfo.h \
    ../../src/common/debug.h \
#    ../../src/common/filelock.h \
    ../../src/common/filepath.h \
#    ../../src/common/schematiclayer.h \
    ../../src/common/smartxmlfile.h \
    ../../src/library/librarybaseelement.h \
#    ../../src/project/circuit/cmd/cmdnetclassadd.h \
#    ../../src/project/circuit/cmd/cmdnetclassremove.h \
#    ../../src/project/circuit/cmd/cmdnetsignaladd.h \
#    ../../src/project/circuit/cmd/cmdnetsignalremove.h \
#    ../../src/common/undocommand.h \
#    ../../src/common/undostack.h \
#    ../../src/project/schematics/symbolinstance.h \
#    ../../src/project/schematics/schematicpagesdock.h \
#    ../../src/common/smartinifile.h \
#    ../../src/project/schematics/cmd/cmdschematicadd.h \
#    ../../src/project/schematics/cmd/cmdschematicremove.h \
#    ../../src/project/erc/ercmsgdock.h \
#    ../../src/project/schematics/fsm/schematiceditorevent.h \
#    ../../src/project/schematics/fsm/ses_select.h \
#    ../../src/project/schematics/fsm/ses_move.h \
#    ../../src/project/schematics/fsm/ses_drawtext.h \
#    ../../src/project/schematics/fsm/ses_drawrect.h \
#    ../../src/project/schematics/fsm/ses_drawpolygon.h \
#    ../../src/project/schematics/fsm/ses_drawcircle.h \
#    ../../src/project/schematics/fsm/ses_drawellipse.h \
#    ../../src/project/schematics/fsm/ses_drawwire.h \
#    ../../src/project/schematics/fsm/ses_addcomponents.h \
#    ../../src/project/schematics/schematicnetpoint.h \
#    ../../src/project/schematics/schematicnetline.h \
#    ../../src/project/schematics/cmd/cmdschematicnetpointadd.h \
#    ../../src/project/schematics/cmd/cmdschematicnetpointremove.h \
#    ../../src/project/schematics/cmd/cmdschematicnetlineadd.h \
#    ../../src/project/schematics/cmd/cmdschematicnetlineremove.h \
#    ../../src/workspace/settings/items/wsi_base.h \
#    ../../src/workspace/settings/items/wsi_applocale.h \
#    ../../src/workspace/settings/items/wsi_projectautosaveinterval.h \
#    ../../src/workspace/settings/items/wsi_librarylocaleorder.h \
#    ../../src/project/circuit/editnetclassesdialog.h \
#    ../../src/project/circuit/cmd/cmdnetclasssetname.h \
#    ../../src/common/dialogs/gridsettingsdialog.h \
#    ../../src/workspace/settings/items/wsi_appdefaultmeasurementunits.h \
#    ../../src/common/application.h \
    ../../src/library/gencompsignal.h \
    ../../src/library/gencompsymbvar.h \
    ../../src/library/gencompsymbvaritem.h \
    ../../src/library/symbolpin.h \
    ../../src/library/symbolpolygon.h \
    ../../src/library/symboltext.h \
#    ../../src/library/symbolgraphicsitem.h \
#    ../../src/project/schematics/symbolpininstance.h \
#    ../../src/library/symbolpingraphicsitem.h \
    ../../src/common/version.h \
#    ../../src/workspace/settings/items/wsi_librarynormorder.h \
#    ../../src/project/schematics/cmd/cmdsymbolinstancemove.h \
#    ../../src/project/circuit/cmd/cmdgencompinstanceadd.h \
#    ../../src/project/schematics/cmd/cmdsymbolinstanceadd.h \
#    ../../src/project/schematics/cmd/cmdschematicnetpointmove.h \
#    ../../src/project/circuit/cmd/cmdgencompsiginstsetnetsignal.h \
#    ../../src/project/schematics/fsm/ses_base.h \
#    ../../src/project/schematics/fsm/ses_fsm.h \
#    ../../src/project/schematics/cmd/cmdsymbolinstanceremove.h \
#    ../../src/project/schematics/cmd/cmdschematicnetpointdetach.h \
#    ../../src/project/schematics/cmd/cmdschematicnetpointsetnetsignal.h \
#    ../../src/workspace/settings/items/wsi_debugtools.h \
#    ../../src/project/erc/ercmsg.h \
#    ../../src/project/erc/ercmsglist.h \
#    ../../src/project/circuit/gencompinstance.h \
    ../../src/library/attribute.h \
#    ../../src/project/circuit/gencompattributeinstance.h \
#    ../../src/common/if_attributeprovider.h \
#    ../../src/project/schematics/symbolinstancepropertiesdialog.h \
#    ../../src/project/circuit/cmd/cmdgencompinstsetname.h \
#    ../../src/project/circuit/cmd/cmdgencompinstsetvalue.h \
#    ../../src/project/circuit/cmd/cmdnetsignalsetname.h \
#    ../../src/project/dialogs/projectpropertieseditordialog.h \
#    ../../src/project/cmd/cmdprojectsetmetadata.h \
#    ../../src/common/smarttextfile.h \
    ../../src/common/smartfile.h \
    ../../src/common/file_io/xmldomdocument.h \
    ../../src/common/file_io/xmldomelement.h \
#    ../../src/project/erc/if_ercmsgprovider.h \
#    ../../src/common/file_io/if_xmlserializableobject.h \
#    ../../src/workspace/settings/items/wsi_appearance.h \
#    ../../src/project/dialogs/addgencompdialog.h \
    ../../src/common/alignment.h
