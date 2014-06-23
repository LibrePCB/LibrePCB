#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

QT += core widgets opengl

TARGET = EDA4U
TEMPLATE = app

CONFIG += c++11

#DEFINES += USE_32BIT_LENGTH_UNITS          # see units.h

RESOURCES += \
    ressources.qrc

SOURCES += \
    src/main.cpp \
    src/common/units.cpp \
    src/common/cadscene.cpp \
    src/common/cadview.cpp \
    src/workspace/workspace.cpp \
    src/library/library.cpp \
    src/project/project.cpp \
    src/workspace/controlpanel/controlpanel.cpp \
    src/workspace/workspacechooserdialog.cpp

HEADERS += \
    src/common/units.h \
    src/common/cadscene.h \
    src/common/cadview.h \
    src/workspace/workspace.h \
    src/library/library.h \
    src/project/project.h \
    src/workspace/controlpanel/controlpanel.h \
    src/workspace/workspacechooserdialog.h

FORMS += \
    src/workspace/controlpanel/controlpanel.ui \
    src/workspace/workspacechooserdialog.ui

