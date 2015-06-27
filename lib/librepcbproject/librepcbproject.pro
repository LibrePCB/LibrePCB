#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbproject

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../

SOURCES += \
    project.cpp \
    schematics/schematiceditor.cpp \
    circuit/circuit.cpp \
    schematics/schematic.cpp \
    boards/board.cpp \
    library/projectlibrary.cpp \
    circuit/netclass.cpp \
    circuit/netsignal.cpp \
    circuit/gencompsignalinstance.cpp \
    circuit/cmd/cmdnetclassadd.cpp \
    circuit/cmd/cmdnetclassremove.cpp \
    circuit/cmd/cmdnetsignaladd.cpp \
    circuit/cmd/cmdnetsignalremove.cpp \
    schematics/schematicpagesdock.cpp \
    schematics/cmd/cmdschematicadd.cpp \
    schematics/cmd/cmdschematicremove.cpp \
    erc/ercmsgdock.cpp \
    schematics/fsm/schematiceditorevent.cpp \
    schematics/fsm/ses_select.cpp \
    schematics/fsm/ses_move.cpp \
    schematics/fsm/ses_drawtext.cpp \
    schematics/fsm/ses_drawrect.cpp \
    schematics/fsm/ses_drawpolygon.cpp \
    schematics/fsm/ses_drawcircle.cpp \
    schematics/fsm/ses_drawellipse.cpp \
    schematics/fsm/ses_drawwire.cpp \
    schematics/fsm/ses_addcomponents.cpp \
    schematics/cmd/cmdschematicnetpointadd.cpp \
    schematics/cmd/cmdschematicnetpointremove.cpp \
    schematics/cmd/cmdschematicnetlineadd.cpp \
    schematics/cmd/cmdschematicnetlineremove.cpp \
    circuit/editnetclassesdialog.cpp \
    schematics/cmd/cmdsymbolinstanceadd.cpp \
    circuit/cmd/cmdgencompsiginstsetnetsignal.cpp \
    schematics/fsm/ses_base.cpp \
    schematics/fsm/ses_fsm.cpp \
    schematics/cmd/cmdsymbolinstanceremove.cpp \
    schematics/cmd/cmdschematicnetpointdetach.cpp \
    erc/ercmsg.cpp \
    erc/ercmsglist.cpp \
    circuit/gencompinstance.cpp \
    circuit/gencompattributeinstance.cpp \
    schematics/symbolinstancepropertiesdialog.cpp \
    dialogs/projectpropertieseditordialog.cpp \
    cmd/cmdprojectsetmetadata.cpp \
    dialogs/addgencompdialog.cpp \
    schematics/fsm/ses_addnetlabel.cpp \
    schematics/cmd/cmdschematicnetlabeladd.cpp \
    schematics/cmd/cmdschematicnetlabelremove.cpp \
    schematics/cmd/cmdschematicnetlabeledit.cpp \
    schematics/schematicclipboard.cpp \
    circuit/cmd/cmdgencompinstedit.cpp \
    circuit/cmd/cmdnetclassedit.cpp \
    circuit/cmd/cmdnetsignaledit.cpp \
    schematics/cmd/cmdschematicnetpointedit.cpp \
    schematics/cmd/cmdsymbolinstanceedit.cpp \
    circuit/cmd/cmdgencompinstadd.cpp \
    circuit/cmd/cmdgencompinstremove.cpp \
    circuit/cmd/cmdgencompattrinstadd.cpp \
    circuit/cmd/cmdgencompattrinstremove.cpp \
    circuit/cmd/cmdgencompattrinstedit.cpp \
    settings/projectsettings.cpp \
    settings/projectsettingsdialog.cpp \
    settings/cmd/cmdprojectsettingschange.cpp \
    schematics/items/si_base.cpp \
    schematics/items/si_netlabel.cpp \
    schematics/items/si_netline.cpp \
    schematics/items/si_netpoint.cpp \
    schematics/items/si_symbol.cpp \
    schematics/items/si_symbolpin.cpp \
    schematics/graphicsitems/sgi_base.cpp \
    schematics/graphicsitems/sgi_netlabel.cpp \
    schematics/graphicsitems/sgi_netline.cpp \
    schematics/graphicsitems/sgi_netpoint.cpp \
    schematics/graphicsitems/sgi_symbol.cpp \
    schematics/graphicsitems/sgi_symbolpin.cpp \
    boards/boardeditor.cpp \
    boards/cmd/cmdboardadd.cpp \
    boards/items/bi_base.cpp \
    boards/componentinstance.cpp \
    boards/items/bi_footprint.cpp \
    boards/graphicsitems/bgi_base.cpp \
    boards/graphicsitems/bgi_footprint.cpp \
    boards/boardlayerprovider.cpp \
    boards/items/bi_footprintpad.cpp \
    boards/graphicsitems/bgi_footprintpad.cpp \
    boards/unplacedcomponentsdock.cpp \
    boards/cmd/cmdcomponentinstanceadd.cpp \
    boards/fsm/bes_base.cpp \
    boards/fsm/boardeditorevent.cpp \
    boards/fsm/bes_fsm.cpp \
    boards/fsm/bes_select.cpp \
    boards/cmd/cmdcomponentinstanceedit.cpp

HEADERS += \
    project.h \
    schematics/schematiceditor.h \
    circuit/circuit.h \
    schematics/schematic.h \
    boards/board.h \
    library/projectlibrary.h \
    circuit/netclass.h \
    circuit/netsignal.h \
    circuit/gencompsignalinstance.h \
    circuit/cmd/cmdnetclassadd.h \
    circuit/cmd/cmdnetclassremove.h \
    circuit/cmd/cmdnetsignaladd.h \
    circuit/cmd/cmdnetsignalremove.h \
    schematics/schematicpagesdock.h \
    schematics/cmd/cmdschematicadd.h \
    schematics/cmd/cmdschematicremove.h \
    erc/ercmsgdock.h \
    schematics/fsm/schematiceditorevent.h \
    schematics/fsm/ses_select.h \
    schematics/fsm/ses_move.h \
    schematics/fsm/ses_drawtext.h \
    schematics/fsm/ses_drawrect.h \
    schematics/fsm/ses_drawpolygon.h \
    schematics/fsm/ses_drawcircle.h \
    schematics/fsm/ses_drawellipse.h \
    schematics/fsm/ses_drawwire.h \
    schematics/fsm/ses_addcomponents.h \
    schematics/cmd/cmdschematicnetpointadd.h \
    schematics/cmd/cmdschematicnetpointremove.h \
    schematics/cmd/cmdschematicnetlineadd.h \
    schematics/cmd/cmdschematicnetlineremove.h \
    circuit/editnetclassesdialog.h \
    schematics/cmd/cmdsymbolinstanceadd.h \
    circuit/cmd/cmdgencompsiginstsetnetsignal.h \
    schematics/fsm/ses_base.h \
    schematics/fsm/ses_fsm.h \
    schematics/cmd/cmdsymbolinstanceremove.h \
    schematics/cmd/cmdschematicnetpointdetach.h \
    erc/ercmsg.h \
    erc/ercmsglist.h \
    circuit/gencompinstance.h \
    circuit/gencompattributeinstance.h \
    schematics/symbolinstancepropertiesdialog.h \
    dialogs/projectpropertieseditordialog.h \
    cmd/cmdprojectsetmetadata.h \
    erc/if_ercmsgprovider.h \
    dialogs/addgencompdialog.h \
    schematics/fsm/ses_addnetlabel.h \
    schematics/cmd/cmdschematicnetlabeladd.h \
    schematics/cmd/cmdschematicnetlabelremove.h \
    schematics/cmd/cmdschematicnetlabeledit.h \
    schematics/schematicclipboard.h \
    circuit/cmd/cmdgencompinstedit.h \
    circuit/cmd/cmdnetclassedit.h \
    circuit/cmd/cmdnetsignaledit.h \
    schematics/cmd/cmdschematicnetpointedit.h \
    schematics/cmd/cmdsymbolinstanceedit.h \
    circuit/cmd/cmdgencompinstadd.h \
    circuit/cmd/cmdgencompinstremove.h \
    circuit/cmd/cmdgencompattrinstadd.h \
    circuit/cmd/cmdgencompattrinstremove.h \
    circuit/cmd/cmdgencompattrinstedit.h \
    settings/projectsettings.h \
    settings/projectsettingsdialog.h \
    settings/cmd/cmdprojectsettingschange.h \
    schematics/items/si_base.h \
    schematics/items/si_netlabel.h \
    schematics/items/si_netline.h \
    schematics/items/si_netpoint.h \
    schematics/items/si_symbol.h \
    schematics/items/si_symbolpin.h \
    schematics/graphicsitems/sgi_base.h \
    schematics/graphicsitems/sgi_netlabel.h \
    schematics/graphicsitems/sgi_netline.h \
    schematics/graphicsitems/sgi_netpoint.h \
    schematics/graphicsitems/sgi_symbol.h \
    schematics/graphicsitems/sgi_symbolpin.h \
    boards/boardeditor.h \
    boards/cmd/cmdboardadd.h \
    boards/items/bi_base.h \
    boards/componentinstance.h \
    boards/items/bi_footprint.h \
    boards/graphicsitems/bgi_base.h \
    boards/graphicsitems/bgi_footprint.h \
    boards/boardlayerprovider.h \
    boards/items/bi_footprintpad.h \
    boards/graphicsitems/bgi_footprintpad.h \
    boards/unplacedcomponentsdock.h \
    boards/cmd/cmdcomponentinstanceadd.h \
    boards/fsm/bes_base.h \
    boards/fsm/boardeditorevent.h \
    boards/fsm/bes_fsm.h \
    boards/fsm/bes_select.h \
    boards/cmd/cmdcomponentinstanceedit.h

FORMS += \
    schematics/schematiceditor.ui \
    schematics/schematicpagesdock.ui \
    erc/ercmsgdock.ui \
    circuit/editnetclassesdialog.ui \
    schematics/symbolinstancepropertiesdialog.ui \
    dialogs/projectpropertieseditordialog.ui \
    dialogs/addgencompdialog.ui \
    settings/projectsettingsdialog.ui \
    boards/boardeditor.ui \
    boards/unplacedcomponentsdock.ui
