#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

TEMPLATE = app
TARGET = librepcb

# Set the path for the generated binary
GENERATED_DIR = ../generated

# Use common project definitions
include(../common.pri)

QT += core widgets opengl network xml printsupport sql

exists(../.git):DEFINES += GIT_BRANCH=\\\"master\\\"

win32 {
    # Windows-specific configurations
    RC_ICONS = ../packaging/windows/img/librepcb.ico
}

macx {
    # Mac-specific configurations
    ICON = ../packaging/mac/img/librepcb.icns
}

unix:!macx {
    # Linux/UNIX-specific configurations
    target.path = $${PREFIX}/bin
    icon.path = $${PREFIX}/share/pixmaps
    icon.files = ../packaging/unix/img/librepcb.svg
    desktop.path = $${PREFIX}/share/applications
    desktop.files = ../packaging/unix/librepcb.desktop
    mimexml.path = $${PREFIX}/share/mime/packages
    mimexml.files = ../packaging/unix/mime/librepcb.xml
    mimedesktop.path = $${PREFIX}/share/mimelnk/application
    mimedesktop.files = ../packaging/unix/mime/x-librepcb-project.desktop
    INSTALLS += target icon desktop mimexml mimedesktop
}

# Note: The order of the libraries is very important for the linker!
# Another order could end up in "undefined reference" errors!
LIBS += \
    -L$${DESTDIR} \
    -lhoedown \
    -llibrepcbprojecteditor \
    -llibrepcblibraryeditor \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \
    -llibrepcbcommon

INCLUDEPATH += \
    ../3rdparty \
    ../libs

DEPENDPATH += \
    ../3rdparty/hoedown \
    ../libs/librepcbprojecteditor \
    ../libs/librepcblibraryeditor \
    ../libs/librepcbworkspace \
    ../libs/librepcbproject \
    ../libs/librepcblibrary \
    ../libs/librepcbcommon

PRE_TARGETDEPS += \
    $${DESTDIR}/libhoedown.a \
    $${DESTDIR}/liblibrepcbprojecteditor.a \
    $${DESTDIR}/liblibrepcblibraryeditor.a \
    $${DESTDIR}/liblibrepcbworkspace.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a

TRANSLATIONS = \
    ../i18n/librepcb_de.ts \
    ../i18n/librepcb_de_CH.ts \
    ../i18n/librepcb_gsw_CH.ts

RESOURCES += \
    ../img/images.qrc \
    ../i18n/translations.qrc

SOURCES += \
    main.cpp \
    controlpanel/controlpanel.cpp \
    firstrunwizard/firstrunwizard.cpp \
    firstrunwizard/firstrunwizardpage_welcome.cpp \
    firstrunwizard/firstrunwizardpage_workspacepath.cpp \
    markdown/markdownconverter.cpp

HEADERS += \
    controlpanel/controlpanel.h \
    firstrunwizard/firstrunwizard.h \
    firstrunwizard/firstrunwizardpage_welcome.h \
    firstrunwizard/firstrunwizardpage_workspacepath.h \
    markdown/markdownconverter.h

FORMS += \
    controlpanel/controlpanel.ui \
    firstrunwizard/firstrunwizard.ui \
    firstrunwizard/firstrunwizardpage_welcome.ui \
    firstrunwizard/firstrunwizardpage_workspacepath.ui


# Custom compiler "lrelease" for qm generation
isEmpty(QMAKE_LRELEASE) {
    win32: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    else: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
lrelease.input = TRANSLATIONS
lrelease.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
lrelease.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
lrelease.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += lrelease
PRE_TARGETDEPS += compiler_lrelease_make_all
