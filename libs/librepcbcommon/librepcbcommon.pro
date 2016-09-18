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

QT += core widgets xml opengl network sql

CONFIG += staticlib

INCLUDEPATH += \
    ../../3rdparty/quazip

# set preprocessor defines
DEFINES += APP_VERSION="\\\"0.1.0\\\""
DEFINES += FILE_FORMAT_VERSION="\\\"0.1\\\""
DEFINES += GIT_VERSION="\\\"$(shell git -C \""$$_PRO_FILE_PWD_"\" describe --abbrev=7 --dirty --always --tags)\\\""
DEFINES += LOCAL_RESOURCES_DIR="\\\"$${LOCAL_RESOURCES_DIR}\\\""
DEFINES += INSTALLED_RESOURCES_DIR="\\\"$${INSTALLED_RESOURCES_DIR}\\\""
DEFINES += INSTALLATION_PREFIX="\\\"$${PREFIX}\\\""
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
    fileio/directorylock.h \
    fileio/filepath.h \
    fileio/if_xmlserializableobject.h \
    fileio/smartfile.h \
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
    if_boardlayerprovider.h \
    uuid.h \
    geometry/polygon.h \
    geometry/ellipse.h \
    geometry/text.h \
    geometry/hole.h \
    undocommandgroup.h \
    scopeguard.h \
    scopeguardlist.h \
    boarddesignrules.h \
    dialogs/boarddesignrulesdialog.h \
    cam/gerbergenerator.h \
    cam/gerberaperturelist.h \
    cam/excellongenerator.h \
    fileio/smartversionfile.h \
    fileio/fileutils.h \
    sqlitedatabase.h

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
    fileio/directorylock.cpp \
    fileio/filepath.cpp \
    fileio/smartfile.cpp \
    fileio/smarttextfile.cpp \
    fileio/smartxmlfile.cpp \
    fileio/xmldomdocument.cpp \
    fileio/xmldomelement.cpp \
    graphics/graphicsitem.cpp \
    graphics/graphicsscene.cpp \
    graphics/graphicsview.cpp \
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
    boardlayer.cpp \
    uuid.cpp \
    geometry/polygon.cpp \
    geometry/ellipse.cpp \
    geometry/text.cpp \
    geometry/hole.cpp \
    undocommandgroup.cpp \
    boarddesignrules.cpp \
    dialogs/boarddesignrulesdialog.cpp \
    cam/gerbergenerator.cpp \
    cam/gerberaperturelist.cpp \
    cam/excellongenerator.cpp \
    fileio/smartversionfile.cpp \
    fileio/fileutils.cpp \
    sqlitedatabase.cpp

FORMS += \
    dialogs/gridsettingsdialog.ui \
    dialogs/boarddesignrulesdialog.ui
