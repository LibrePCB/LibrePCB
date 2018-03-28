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

# Set preprocessor defines
DEFINES += APP_VERSION="\\\"0.1.0\\\""
DEFINES += FILE_FORMAT_VERSION="\\\"0.1\\\""
DEFINES += GIT_VERSION="\\\"$(shell git -C \""$$_PRO_FILE_PWD_"\" describe --abbrev=7 --dirty --always --tags)\\\""
#DEFINES += USE_32BIT_LENGTH_UNITS          # see units/length.h

QT += core widgets xml opengl network sql

CONFIG += staticlib

INCLUDEPATH += \
    ../../fontobene \
    ../../quazip \
    ../../sexpresso \
    ../../ \

SOURCES += \
    alignment.cpp \
    application.cpp \
    attributes/attribute.cpp \
    attributes/attributeprovider.cpp \
    attributes/attributesubstitutor.cpp \
    attributes/attributetype.cpp \
    attributes/attributeunit.cpp \
    attributes/attrtypecapacitance.cpp \
    attributes/attrtypefrequency.cpp \
    attributes/attrtypeinductance.cpp \
    attributes/attrtyperesistance.cpp \
    attributes/attrtypestring.cpp \
    attributes/attrtypevoltage.cpp \
    boarddesignrules.cpp \
    cam/excellongenerator.cpp \
    cam/gerberaperturelist.cpp \
    cam/gerbergenerator.cpp \
    debug.cpp \
    dialogs/boarddesignrulesdialog.cpp \
    dialogs/ellipsepropertiesdialog.cpp \
    dialogs/gridsettingsdialog.cpp \
    dialogs/holepropertiesdialog.cpp \
    dialogs/polygonpropertiesdialog.cpp \
    dialogs/textpropertiesdialog.cpp \
    exceptions.cpp \
    fileio/directorylock.cpp \
    fileio/filepath.cpp \
    fileio/fileutils.cpp \
    fileio/sexpression.cpp \
    fileio/smartfile.cpp \
    fileio/smartsexprfile.cpp \
    fileio/smarttextfile.cpp \
    fileio/smartversionfile.cpp \
    font/strokefont.cpp \
    font/strokefontpool.cpp \
    geometry/cmd/cmdellipseedit.cpp \
    geometry/cmd/cmdholeedit.cpp \
    geometry/cmd/cmdpolygonedit.cpp \
    geometry/cmd/cmdtextedit.cpp \
    geometry/ellipse.cpp \
    geometry/hole.cpp \
    geometry/path.cpp \
    geometry/polygon.cpp \
    geometry/text.cpp \
    geometry/vertex.cpp \
    graphics/defaultgraphicslayerprovider.cpp \
    graphics/ellipsegraphicsitem.cpp \
    graphics/graphicslayer.cpp \
    graphics/graphicsscene.cpp \
    graphics/graphicsview.cpp \
    graphics/holegraphicsitem.cpp \
    graphics/linegraphicsitem.cpp \
    graphics/origincrossgraphicsitem.cpp \
    graphics/polygongraphicsitem.cpp \
    graphics/primitiveellipsegraphicsitem.cpp \
    graphics/primitivepathgraphicsitem.cpp \
    graphics/primitivetextgraphicsitem.cpp \
    graphics/textgraphicsitem.cpp \
    gridproperties.cpp \
    network/filedownload.cpp \
    network/networkaccessmanager.cpp \
    network/networkrequest.cpp \
    network/networkrequestbase.cpp \
    network/repository.cpp \
    signalrole.cpp \
    sqlitedatabase.cpp \
    systeminfo.cpp \
    toolbox.cpp \
    undocommand.cpp \
    undocommandgroup.cpp \
    undostack.cpp \
    units/angle.cpp \
    units/length.cpp \
    units/lengthunit.cpp \
    units/point.cpp \
    units/ratio.cpp \
    utils/clipperhelpers.cpp \
    utils/exclusiveactiongroup.cpp \
    utils/graphicslayerstackappearancesettings.cpp \
    utils/toolbarproxy.cpp \
    utils/undostackactiongroup.cpp \
    uuid.cpp \
    version.cpp \
    widgets/alignmentselector.cpp \
    widgets/attributelisteditorwidget.cpp \
    widgets/attributetypecombobox.cpp \
    widgets/attributeunitcombobox.cpp \
    widgets/centeredcheckbox.cpp \
    widgets/graphicslayercombobox.cpp \
    widgets/patheditorwidget.cpp \
    widgets/signalrolecombobox.cpp \
    widgets/statusbar.cpp \

HEADERS += \
    alignment.h \
    application.h \
    attributes/attribute.h \
    attributes/attributeprovider.h \
    attributes/attributesubstitutor.h \
    attributes/attributetype.h \
    attributes/attributeunit.h \
    attributes/attrtypecapacitance.h \
    attributes/attrtypefrequency.h \
    attributes/attrtypeinductance.h \
    attributes/attrtyperesistance.h \
    attributes/attrtypestring.h \
    attributes/attrtypevoltage.h \
    boarddesignrules.h \
    cam/excellongenerator.h \
    cam/gerberaperturelist.h \
    cam/gerbergenerator.h \
    debug.h \
    dialogs/boarddesignrulesdialog.h \
    dialogs/ellipsepropertiesdialog.h \
    dialogs/gridsettingsdialog.h \
    dialogs/holepropertiesdialog.h \
    dialogs/polygonpropertiesdialog.h \
    dialogs/textpropertiesdialog.h \
    exceptions.h \
    fileio/cmd/cmdlistelementinsert.h \
    fileio/cmd/cmdlistelementremove.h \
    fileio/cmd/cmdlistelementsswap.h \
    fileio/directorylock.h \
    fileio/filepath.h \
    fileio/fileutils.h \
    fileio/serializablekeyvaluemap.h \
    fileio/serializableobject.h \
    fileio/serializableobjectlist.h \
    fileio/sexpression.h \
    fileio/smartfile.h \
    fileio/smartsexprfile.h \
    fileio/smarttextfile.h \
    fileio/smartversionfile.h \
    font/strokefont.h \
    font/strokefontpool.h \
    geometry/cmd/cmdellipseedit.h \
    geometry/cmd/cmdholeedit.h \
    geometry/cmd/cmdpolygonedit.h \
    geometry/cmd/cmdtextedit.h \
    geometry/ellipse.h \
    geometry/hole.h \
    geometry/path.h \
    geometry/polygon.h \
    geometry/text.h \
    geometry/vertex.h \
    graphics/defaultgraphicslayerprovider.h \
    graphics/ellipsegraphicsitem.h \
    graphics/graphicslayer.h \
    graphics/graphicsscene.h \
    graphics/graphicsview.h \
    graphics/holegraphicsitem.h \
    graphics/if_graphicsvieweventhandler.h \
    graphics/linegraphicsitem.h \
    graphics/origincrossgraphicsitem.h \
    graphics/polygongraphicsitem.h \
    graphics/primitiveellipsegraphicsitem.h \
    graphics/primitivepathgraphicsitem.h \
    graphics/primitivetextgraphicsitem.h \
    graphics/textgraphicsitem.h \
    gridproperties.h \
    network/filedownload.h \
    network/networkaccessmanager.h \
    network/networkrequest.h \
    network/networkrequestbase.h \
    network/repository.h \
    scopeguard.h \
    scopeguardlist.h \
    signalrole.h \
    sqlitedatabase.h \
    systeminfo.h \
    toolbox.h \
    undocommand.h \
    undocommandgroup.h \
    undostack.h \
    units/all_length_units.h \
    units/angle.h \
    units/length.h \
    units/lengthunit.h \
    units/point.h \
    units/ratio.h \
    utils/clipperhelpers.h \
    utils/exclusiveactiongroup.h \
    utils/graphicslayerstackappearancesettings.h \
    utils/toolbarproxy.h \
    utils/undostackactiongroup.h \
    uuid.h \
    version.h \
    widgets/alignmentselector.h \
    widgets/attributelisteditorwidget.h \
    widgets/attributetypecombobox.h \
    widgets/attributeunitcombobox.h \
    widgets/centeredcheckbox.h \
    widgets/graphicslayercombobox.h \
    widgets/patheditorwidget.h \
    widgets/signalrolecombobox.h \
    widgets/statusbar.h \

FORMS += \
    dialogs/boarddesignrulesdialog.ui \
    dialogs/ellipsepropertiesdialog.ui \
    dialogs/gridsettingsdialog.ui \
    dialogs/holepropertiesdialog.ui \
    dialogs/polygonpropertiesdialog.ui \
    dialogs/textpropertiesdialog.ui \
    widgets/alignmentselector.ui \

