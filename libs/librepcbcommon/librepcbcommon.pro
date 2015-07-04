#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:55:49
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbcommon

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

QT += core widgets xml opengl network

CONFIG += staticlib

#DEFINES += USE_32BIT_LENGTH_UNITS          # see units/length.h

HEADERS += \
    attributes/attributetype.h \
    attributes/attributeunit.h \
    attributes/attrtypecapacitance.h \
    attributes/attrtypefrequency.h \
    attributes/attrtypeinductance.h \
    attributes/attrtyperesistance.h \
    attributes/attrtypestring.h \
    attributes/attrtypevoltage.h \
    dialogs/gridsettingsdialog.h \
    fileio/filelock.h \
    fileio/filepath.h \
    fileio/if_xmlserializableobject.h \
    fileio/smartfile.h \
    fileio/smartinifile.h \
    fileio/smarttextfile.h \
    fileio/smartxmlfile.h \
    fileio/xmldomdocument.h \
    fileio/xmldomelement.h \
    graphics/graphicsitem.h \
    graphics/graphicsscene.h \
    graphics/graphicsview.h \
    graphics/if_graphicsvieweventhandler.h \
    units/all_length_units.h \
    units/angle.h \
    units/length.h \
    units/lengthunit.h \
    units/point.h \
    alignment.h \
    application.h \
    debug.h \
    exceptions.h \
    gridproperties.h \
    if_attributeprovider.h \
    schematiclayer.h \
    systeminfo.h \
    undocommand.h \
    undostack.h \
    version.h \
    if_schematiclayerprovider.h \
    boardlayer.h \
    if_boardlayerprovider.h

SOURCES += \
    attributes/attributetype.cpp \
    attributes/attributeunit.cpp \
    attributes/attrtypecapacitance.cpp \
    attributes/attrtypefrequency.cpp \
    attributes/attrtypeinductance.cpp \
    attributes/attrtyperesistance.cpp \
    attributes/attrtypestring.cpp \
    attributes/attrtypevoltage.cpp \
    dialogs/gridsettingsdialog.cpp \
    fileio/filelock.cpp \
    fileio/filepath.cpp \
    fileio/smartfile.cpp \
    fileio/smartinifile.cpp \
    fileio/smarttextfile.cpp \
    fileio/smartxmlfile.cpp \
    fileio/xmldomdocument.cpp \
    fileio/xmldomelement.cpp \
    graphics/graphicsitem.cpp \
    graphics/graphicsscene.cpp \
    graphics/graphicsview.cpp \
    graphics/if_graphicsvieweventhandler.cpp \
    units/angle.cpp \
    units/length.cpp \
    units/lengthunit.cpp \
    units/point.cpp \
    alignment.cpp \
    application.cpp \
    debug.cpp \
    exceptions.cpp \
    gridproperties.cpp \
    if_attributeprovider.cpp \
    schematiclayer.cpp \
    systeminfo.cpp \
    undocommand.cpp \
    undostack.cpp \
    version.cpp \
    boardlayer.cpp

FORMS += \
    dialogs/gridsettingsdialog.ui
