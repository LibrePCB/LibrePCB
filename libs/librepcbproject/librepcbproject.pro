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
    circuit/circuit.cpp \
    circuit/netclass.cpp \
    circuit/netsignal.cpp \
    circuit/gencompsignalinstance.cpp \
    circuit/gencompinstance.cpp \
    circuit/gencompattributeinstance.cpp \
    circuit/cmd/cmdnetclassadd.cpp \
    circuit/cmd/cmdnetclassremove.cpp \
    circuit/cmd/cmdnetsignaladd.cpp \
    circuit/cmd/cmdnetsignalremove.cpp \
    circuit/cmd/cmdgencompsiginstsetnetsignal.cpp \
    circuit/cmd/cmdgencompinstedit.cpp \
    circuit/cmd/cmdnetclassedit.cpp \
    circuit/cmd/cmdnetsignaledit.cpp \
    circuit/cmd/cmdgencompinstadd.cpp \
    circuit/cmd/cmdgencompinstremove.cpp \
    circuit/cmd/cmdgencompattrinstadd.cpp \
    circuit/cmd/cmdgencompattrinstremove.cpp \
    circuit/cmd/cmdgencompattrinstedit.cpp \
    schematics/schematic.cpp \
    schematics/cmd/cmdschematicadd.cpp \
    schematics/cmd/cmdschematicremove.cpp \
    schematics/cmd/cmdschematicnetpointadd.cpp \
    schematics/cmd/cmdschematicnetpointremove.cpp \
    schematics/cmd/cmdschematicnetlineadd.cpp \
    schematics/cmd/cmdschematicnetlineremove.cpp \
    schematics/cmd/cmdsymbolinstanceadd.cpp \
    schematics/cmd/cmdsymbolinstanceremove.cpp \
    schematics/cmd/cmdschematicnetpointdetach.cpp \
    schematics/cmd/cmdschematicnetlabeladd.cpp \
    schematics/cmd/cmdschematicnetlabelremove.cpp \
    schematics/cmd/cmdschematicnetlabeledit.cpp \
    schematics/cmd/cmdschematicnetpointedit.cpp \
    schematics/cmd/cmdsymbolinstanceedit.cpp \
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
    boards/board.cpp \
    boards/cmd/cmdboardadd.cpp \
    boards/items/bi_base.cpp \
    boards/items/bi_footprint.cpp \
    boards/graphicsitems/bgi_base.cpp \
    boards/graphicsitems/bgi_footprint.cpp \
    boards/boardlayerprovider.cpp \
    boards/items/bi_footprintpad.cpp \
    boards/graphicsitems/bgi_footprintpad.cpp \
    library/projectlibrary.cpp \
    erc/ercmsg.cpp \
    erc/ercmsglist.cpp \
    cmd/cmdprojectsetmetadata.cpp \
    settings/projectsettings.cpp \
    settings/cmd/cmdprojectsettingschange.cpp \
    schematics/schematiclayerprovider.cpp \
    library/cmd/cmdprojectlibraryaddelement.cpp \
    boards/deviceinstance.cpp \
    boards/cmd/cmddeviceinstanceadd.cpp \
    boards/cmd/cmddeviceinstanceedit.cpp \
    boards/cmd/cmddeviceinstanceremove.cpp

HEADERS += \
    project.h \
    circuit/circuit.h \
    circuit/netclass.h \
    circuit/netsignal.h \
    circuit/gencompsignalinstance.h \
    circuit/gencompinstance.h \
    circuit/gencompattributeinstance.h \
    circuit/cmd/cmdnetclassadd.h \
    circuit/cmd/cmdnetclassremove.h \
    circuit/cmd/cmdnetsignaladd.h \
    circuit/cmd/cmdnetsignalremove.h \
    circuit/cmd/cmdgencompsiginstsetnetsignal.h \
    circuit/cmd/cmdgencompinstedit.h \
    circuit/cmd/cmdnetclassedit.h \
    circuit/cmd/cmdnetsignaledit.h \
    circuit/cmd/cmdgencompinstadd.h \
    circuit/cmd/cmdgencompinstremove.h \
    circuit/cmd/cmdgencompattrinstadd.h \
    circuit/cmd/cmdgencompattrinstremove.h \
    circuit/cmd/cmdgencompattrinstedit.h \
    schematics/schematic.h \
    schematics/cmd/cmdschematicadd.h \
    schematics/cmd/cmdschematicremove.h \
    schematics/cmd/cmdschematicnetpointadd.h \
    schematics/cmd/cmdschematicnetpointremove.h \
    schematics/cmd/cmdschematicnetlineadd.h \
    schematics/cmd/cmdschematicnetlineremove.h \
    schematics/cmd/cmdsymbolinstanceadd.h \
    schematics/cmd/cmdsymbolinstanceremove.h \
    schematics/cmd/cmdschematicnetpointdetach.h \
    schematics/cmd/cmdschematicnetlabeladd.h \
    schematics/cmd/cmdschematicnetlabelremove.h \
    schematics/cmd/cmdschematicnetlabeledit.h \
    schematics/cmd/cmdschematicnetpointedit.h \
    schematics/cmd/cmdsymbolinstanceedit.h \
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
    boards/board.h \
    boards/cmd/cmdboardadd.h \
    boards/items/bi_base.h \
    boards/items/bi_footprint.h \
    boards/graphicsitems/bgi_base.h \
    boards/graphicsitems/bgi_footprint.h \
    boards/boardlayerprovider.h \
    boards/items/bi_footprintpad.h \
    boards/graphicsitems/bgi_footprintpad.h \
    library/projectlibrary.h \
    erc/ercmsg.h \
    erc/ercmsglist.h \
    erc/if_ercmsgprovider.h \
    cmd/cmdprojectsetmetadata.h \
    settings/projectsettings.h \
    settings/cmd/cmdprojectsettingschange.h \
    schematics/schematiclayerprovider.h \
    library/cmd/cmdprojectlibraryaddelement.h \
    boards/deviceinstance.h \
    boards/cmd/cmddeviceinstanceadd.h \
    boards/cmd/cmddeviceinstanceremove.h \
    boards/cmd/cmddeviceinstanceedit.h

FORMS +=
