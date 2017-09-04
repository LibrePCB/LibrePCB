#-------------------------------------------------
#
# Project created 2014-08-02
#
#-------------------------------------------------

TEMPLATE = app
TARGET = tests

# Set the path for the generated binary
GENERATED_DIR = ../generated

# Use common project definitions
include(../common.pri)

# Set preprocessor defines
DEFINES += TEST_DATA_DIR=\\\"$${PWD}/data\\\"

QT += core widgets network printsupport xml opengl sql concurrent

CONFIG += console
CONFIG -= app_bundle

# ignore warnings produced by macros from gtest
# TODO: fix these warnings because actually the code is not valid!
QMAKE_CXXFLAGS += -Wno-gnu-zero-variadic-macro-arguments
QMAKE_CXXFLAGS_DEBUG += -Wno-gnu-zero-variadic-macro-arguments

LIBS += \
    -L$${DESTDIR} \
    -lgoogletest \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon \     # Another order could end up in "undefined reference" errors!
    -lquazip -lz

INCLUDEPATH += \
    ../libs/googletest/googletest/include \
    ../libs/googletest/googlemock/include \
    ../libs/quazip \
    ../libs

DEPENDPATH += \
    ../libs/librepcb/workspace \
    ../libs/librepcb/project \
    ../libs/librepcb/library \
    ../libs/librepcb/common \
    ../libs/quazip \

PRE_TARGETDEPS += \
    $${DESTDIR}/libgoogletest.a \
    $${DESTDIR}/liblibrepcbworkspace.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libquazip.a

SOURCES += \
    common/applicationtest.cpp \
    common/directorylocktest.cpp \
    common/filedownloadtest.cpp \
    common/fileio/serializableobjectlisttest.cpp \
    common/filepathtest.cpp \
    common/networkrequesttest.cpp \
    common/pointtest.cpp \
    common/ratiotest.cpp \
    common/scopeguardtest.cpp \
    common/sqlitedatabasetest.cpp \
    common/systeminfotest.cpp \
    common/uuidtest.cpp \
    common/versiontest.cpp \
    main.cpp \
    project/projecttest.cpp \
    workspace/workspacetest.cpp \

HEADERS += \
    common/fileio/serializableobjectmock.h \
    common/networkrequestbasesignalreceiver.h \

FORMS += \

