#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

TEMPLATE = app
TARGET = librepcb

# Use common project definitions
include(../../common.pri)

QT += core widgets opengl network xml printsupport sql

win32 {
    # Windows-specific configurations
    RC_ICONS = ../../img/app/librepcb.ico
}

macx {
    # Mac-specific configurations
    ICON = ../../img/app/librepcb.icns
}

unix:!macx {
    # Linux/UNIX-specific configurations
}

# Files to be installed by "make install"
target.path = $${PREFIX}/bin
share.path = $${PREFIX}
share.files = $${SHARE_DIR_ABS}
INSTALLS += target share

# Note: The order of the libraries is very important for the linker!
# Another order could end up in "undefined reference" errors!
LIBS += \
    -L$${DESTDIR} \
    -lhoedown \
    -llibrepcblibrarymanager \
    -llibrepcbprojecteditor \
    -llibrepcblibraryeditor \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \
    -llibrepcbcommon \
    -lsexpresso \
    -lclipper \
    -lquazip -lz

INCLUDEPATH += \
    ../../libs \
    ../../libs/quazip \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/hoedown \
    ../../libs/librepcb/librarymanager \
    ../../libs/librepcb/projecteditor \
    ../../libs/librepcb/libraryeditor \
    ../../libs/librepcb/workspace \
    ../../libs/librepcb/project \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/quazip \
    ../../libs/sexpresso \
    ../../libs/clipper \

PRE_TARGETDEPS += \
    $${DESTDIR}/libhoedown.a \
    $${DESTDIR}/liblibrepcblibrarymanager.a \
    $${DESTDIR}/liblibrepcbprojecteditor.a \
    $${DESTDIR}/liblibrepcblibraryeditor.a \
    $${DESTDIR}/liblibrepcbworkspace.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libquazip.a \
    $${DESTDIR}/libsexpresso.a \
    $${DESTDIR}/libclipper.a \

RESOURCES += \
    ../../img/images.qrc \

SOURCES += \
    controlpanel/controlpanel.cpp \
    firstrunwizard/firstrunwizard.cpp \
    firstrunwizard/firstrunwizardpage_welcome.cpp \
    firstrunwizard/firstrunwizardpage_workspacepath.cpp \
    firstrunwizard/firstrunwizardpage_workspacesettings.cpp \
    main.cpp \
    markdown/markdownconverter.cpp \
    projectlibraryupdater/projectlibraryupdater.cpp \

HEADERS += \
    controlpanel/controlpanel.h \
    firstrunwizard/firstrunwizard.h \
    firstrunwizard/firstrunwizardpage_welcome.h \
    firstrunwizard/firstrunwizardpage_workspacepath.h \
    firstrunwizard/firstrunwizardpage_workspacesettings.h \
    markdown/markdownconverter.h \
    projectlibraryupdater/projectlibraryupdater.h \

FORMS += \
    controlpanel/controlpanel.ui \
    firstrunwizard/firstrunwizard.ui \
    firstrunwizard/firstrunwizardpage_welcome.ui \
    firstrunwizard/firstrunwizardpage_workspacepath.ui \
    firstrunwizard/firstrunwizardpage_workspacesettings.ui \
    projectlibraryupdater/projectlibraryupdater.ui \

# Custom compiler "lrelease" for qm generation
isEmpty(QMAKE_LRELEASE): QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease

TS_FILES = $$files(../../i18n/librepcb_*.ts)
lrelease.input = TS_FILES
lrelease.output = $$SHARE_DIR_ABS/librepcb/i18n/${QMAKE_FILE_BASE}.qm
lrelease.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm $$SHARE_DIR_ABS/librepcb/i18n/${QMAKE_FILE_BASE}.qm
lrelease.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += lrelease
