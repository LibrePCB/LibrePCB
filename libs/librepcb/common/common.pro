#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:55:49
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbcommon

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml opengl network sql

CONFIG += staticlib

INCLUDEPATH += \
    ../../quazip

# set preprocessor defines
DEFINES += APP_VERSION="\\\"0.1.0\\\""
DEFINES += FILE_FORMAT_VERSION="\\\"0.1\\\""
DEFINES += GIT_VERSION="\\\"$(shell git -C \""$$_PRO_FILE_PWD_"\" describe --abbrev=7 --dirty --always --tags)\\\""
#DEFINES += USE_32BIT_LENGTH_UNITS          # see units/length.h

HEADERS += \
    alignment.h \
    application.h \
    attributes/attribute.h \
    attributes/attributelist.h \
    attributes/attributetype.h \
    attributes/attributeunit.h \
    attributes/attrtypecapacitance.h \
    attributes/attrtypefrequency.h \
    attributes/attrtypeinductance.h \
    attributes/attrtyperesistance.h \
    attributes/attrtypestring.h \
    attributes/attrtypevoltage.h \
    boarddesignrules.h \
    boardlayer.h \
    cam/excellongenerator.h \
    cam/gerberaperturelist.h \
    cam/gerbergenerator.h \
    debug.h \
    dialogs/boarddesignrulesdialog.h \
    dialogs/gridsettingsdialog.h \
    exceptions.h \
    fileio/directorylock.h \
    fileio/filepath.h \
    fileio/fileutils.h \
    fileio/if_xmlserializableobject.h \
    fileio/smartfile.h \
    fileio/smarttextfile.h \
    fileio/smartversionfile.h \
    fileio/smartxmlfile.h \
    fileio/xmldomdocument.h \
    fileio/xmldomelement.h \
    geometry/ellipse.h \
    geometry/hole.h \
    geometry/polygon.h \
    geometry/text.h \
    graphics/graphicsitem.h \
    graphics/graphicsscene.h \
    graphics/graphicsview.h \
    graphics/if_graphicsvieweventhandler.h \
    gridproperties.h \
    if_attributeprovider.h \
    if_boardlayerprovider.h \
    if_schematiclayerprovider.h \
    network/filedownload.h \
    network/networkaccessmanager.h \
    network/networkrequest.h \
    network/networkrequestbase.h \
    network/repository.h \
    schematiclayer.h \
    scopeguard.h \
    scopeguardlist.h \
    sqlitedatabase.h \
    systeminfo.h \
    undocommand.h \
    undocommandgroup.h \
    undostack.h \
    units/all_length_units.h \
    units/angle.h \
    units/length.h \
    units/lengthunit.h \
    units/point.h \
    utils/undostackactiongroup.h \
    uuid.h \
    version.h \
    widgets/attributelisteditorwidget.h \
    widgets/attributetypecombobox.h \
    widgets/attributeunitcombobox.h \

SOURCES += \
    alignment.cpp \
    application.cpp \
    attributes/attribute.cpp \
    attributes/attributelist.cpp \
    attributes/attributetype.cpp \
    attributes/attributeunit.cpp \
    attributes/attrtypecapacitance.cpp \
    attributes/attrtypefrequency.cpp \
    attributes/attrtypeinductance.cpp \
    attributes/attrtyperesistance.cpp \
    attributes/attrtypestring.cpp \
    attributes/attrtypevoltage.cpp \
    boarddesignrules.cpp \
    boardlayer.cpp \
    cam/excellongenerator.cpp \
    cam/gerberaperturelist.cpp \
    cam/gerbergenerator.cpp \
    debug.cpp \
    dialogs/boarddesignrulesdialog.cpp \
    dialogs/gridsettingsdialog.cpp \
    exceptions.cpp \
    fileio/directorylock.cpp \
    fileio/filepath.cpp \
    fileio/fileutils.cpp \
    fileio/smartfile.cpp \
    fileio/smarttextfile.cpp \
    fileio/smartversionfile.cpp \
    fileio/smartxmlfile.cpp \
    fileio/xmldomdocument.cpp \
    fileio/xmldomelement.cpp \
    geometry/ellipse.cpp \
    geometry/hole.cpp \
    geometry/polygon.cpp \
    geometry/text.cpp \
    graphics/graphicsitem.cpp \
    graphics/graphicsscene.cpp \
    graphics/graphicsview.cpp \
    gridproperties.cpp \
    if_attributeprovider.cpp \
    network/filedownload.cpp \
    network/networkaccessmanager.cpp \
    network/networkrequest.cpp \
    network/networkrequestbase.cpp \
    network/repository.cpp \
    schematiclayer.cpp \
    sqlitedatabase.cpp \
    systeminfo.cpp \
    undocommand.cpp \
    undocommandgroup.cpp \
    undostack.cpp \
    units/angle.cpp \
    units/length.cpp \
    units/lengthunit.cpp \
    units/point.cpp \
    utils/undostackactiongroup.cpp \
    uuid.cpp \
    version.cpp \
    widgets/attributelisteditorwidget.cpp \
    widgets/attributetypecombobox.cpp \
    widgets/attributeunitcombobox.cpp \

FORMS += \
    dialogs/boarddesignrulesdialog.ui \
    dialogs/gridsettingsdialog.ui \

