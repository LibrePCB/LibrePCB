#-------------------------------------------------
#
# Project created 2014-08-02
#
#-------------------------------------------------

TEMPLATE = app
TARGET = librepcb-unittests

# Use common project definitions
include(../../common.pri)

# Set preprocessor defines
DEFINES += TEST_DATA_DIR=\\\"$${PWD}/../data\\\"

QT += core widgets network printsupport xml opengl sql concurrent

CONFIG += console
CONFIG -= app_bundle

LIBS += \
    -L$${DESTDIR} \
    -lgoogletest \
    -llibrepcbeagleimport \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon \     # Another order could end up in "undefined reference" errors!
    -lsexpresso \
    -lclipper \
    -lmuparser \
    -lparseagle -lquazip -lz

INCLUDEPATH += \
    ../../libs \
    ../../libs/googletest/googletest/include \
    ../../libs/googletest/googlemock/include \
    ../../libs/parseagle \
    ../../libs/quazip \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/librepcb/eagleimport \
    ../../libs/librepcb/workspace \
    ../../libs/librepcb/project \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/parseagle \
    ../../libs/quazip \
    ../../libs/sexpresso \
    ../../libs/clipper \
    ../../libs/muparser \

PRE_TARGETDEPS += \
    $${DESTDIR}/libgoogletest.a \
    $${DESTDIR}/liblibrepcbeagleimport.a \
    $${DESTDIR}/liblibrepcbworkspace.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libquazip.a \
    $${DESTDIR}/libsexpresso.a \
    $${DESTDIR}/libclipper.a \
    $${DESTDIR}/libmuparser.a \

SOURCES += \
    common/algorithm/airwiresbuildertest.cpp \
    common/alignmenttest.cpp \
    common/applicationtest.cpp \
    common/attributes/attributekeytest.cpp \
    common/attributes/attributesubstitutortest.cpp \
    common/circuitidentifiertest.cpp \
    common/fileio/csvfiletest.cpp \
    common/fileio/directorylocktest.cpp \
    common/fileio/filepathtest.cpp \
    common/fileio/serializableobjectlisttest.cpp \
    common/fileio/transactionaldirectorytest.cpp \
    common/fileio/transactionalfilesystemtest.cpp \
    common/geometry/pathmodeltest.cpp \
    common/geometry/pathtest.cpp \
    common/graphics/graphicslayernametest.cpp \
    common/network/filedownloadtest.cpp \
    common/network/networkrequesttest.cpp \
    common/pnp/pickplacecsvwritertest.cpp \
    common/scopeguardtest.cpp \
    common/signalslottest.cpp \
    common/sqlitedatabasetest.cpp \
    common/systeminfotest.cpp \
    common/toolboxtest.cpp \
    common/units/angletest.cpp \
    common/units/lengthsnaptest.cpp \
    common/units/lengthtest.cpp \
    common/units/pointtest.cpp \
    common/units/ratiotest.cpp \
    common/utils/mathparsertest.cpp \
    common/uuidtest.cpp \
    common/versiontest.cpp \
    common/widgets/editabletablewidgettest.cpp \
    eagleimport/deviceconvertertest.cpp \
    eagleimport/devicesetconvertertest.cpp \
    eagleimport/packageconvertertest.cpp \
    eagleimport/symbolconvertertest.cpp \
    library/cmp/componentprefixtest.cpp \
    library/cmp/componentsymbolvariantitemsuffixtest.cpp \
    library/cmp/componentsymbolvariantitemtest.cpp \
    library/librarybaseelementtest.cpp \
    main.cpp \
    project/boards/boardgerberexporttest.cpp \
    project/boards/boardpickplacegeneratortest.cpp \
    project/boards/boardplanefragmentsbuildertest.cpp \
    project/library/projectlibrarytest.cpp \
    project/projecttest.cpp \
    workspace/workspacetest.cpp \

HEADERS += \
    common/attributes/attributeproviderdummy.h \
    common/fileio/serializableobjectmock.h \
    common/network/networkrequestbasesignalreceiver.h \
    common/widgets/editabletablewidgetreceiver.h \

FORMS += \

