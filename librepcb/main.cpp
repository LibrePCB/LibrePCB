/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <QTranslator>
#include <librepcbcommon/application.h>
#include <librepcbcommon/debug.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbworkspace/workspace.h>
#include "firstrunwizard/firstrunwizard.h"
#include "controlpanel/controlpanel.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
using namespace librepcb;
using namespace librepcb::workspace;

/*****************************************************************************************
 *  Function Prototypes
 ****************************************************************************************/

static void setApplicationMetadata() noexcept;
static void writeLogHeader() noexcept;
static void installTranslations() noexcept;
static void init3rdPartyLibs() noexcept;
static void cleanup3rdPartyLibs() noexcept;
static FilePath determineWorkspacePath() noexcept;
static int openWorkspace(const FilePath& path) noexcept;
static int appExec() noexcept;

/*****************************************************************************************
 *  main()
 ****************************************************************************************/

int main(int argc, char* argv[])
{
    Application app(argc, argv);

    // --------------------------------- INITIALIZATION ----------------------------------

    // Set the organization / application names must be done very early because some other
    // classes will use these values (for example QSettings, Debug)!
    setApplicationMetadata();

    // Creates the Debug object which installs the message handler. This must be done as
    // early as possible, but *after* setting application metadata (organization + name).
    Debug::instance();

    // Write some information about the application instance to the log.
    writeLogHeader();

    // Install translation files. This must be done before any widget is shown.
    installTranslations();

    // This is to remove the ugly frames around widgets in all status bars...
    // (from http://www.qtcentre.org/threads/1904)
    app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

    // Initialize all 3rd party libraries
    init3rdPartyLibs();

    // --------------------------------- OPEN WORKSPACE ----------------------------------

    // Get the path of the workspace to open (may show the first run wizard)
    FilePath wsPath = determineWorkspacePath();

    // Open the workspace and catch the return value
    int retval = 0;
    if (wsPath.isValid()) {
        retval = openWorkspace(wsPath);
    }

    // -------------------------------- EXIT APPLICATION ---------------------------------

    // Cleanup all 3rd party libraries
    cleanup3rdPartyLibs();

    return retval;
}

/*****************************************************************************************
 *  setApplicationMetadata()
 ****************************************************************************************/

static void setApplicationMetadata() noexcept
{
    Application::setOrganizationName("LibrePCB");
    Application::setOrganizationDomain("librepcb.org");
#ifdef GIT_BRANCH
    Application::setApplicationName(QString("LibrePCB_git-%1").arg(GIT_BRANCH));
#else
    Application::setApplicationName("LibrePCB");
#endif
    Application::setApplicationVersion(Version(QString("%1.%2.%3").arg(APP_VERSION_MAJOR)
                                       .arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH)));
}

/*****************************************************************************************
 *  writeLogHeader()
 ****************************************************************************************/

static void writeLogHeader() noexcept
{
    // @TODO: After removing support for Qt versions below 5.5, we could use qInfo() here.

    // write application name and version to log
    QString msg = QString("LibrePCB %1 (%2)").arg(qApp->applicationVersion(), GIT_VERSION);
    Debug::instance()->print(Debug::DebugLevel_t::Info, msg, __FILE__, __LINE__);

    // write Qt version to log
    msg = QString("Qt version: %1 (compiled against %2)").arg(qVersion(), QT_VERSION_STR);
    Debug::instance()->print(Debug::DebugLevel_t::Info, msg, __FILE__, __LINE__);

    // write resources directory path to log
    msg = QString("Resources directory: %1").arg(Application::getResourcesDir().toNative());
    Debug::instance()->print(Debug::DebugLevel_t::Info, msg, __FILE__, __LINE__);
}

/*****************************************************************************************
 *  installTranslations()
 ****************************************************************************************/

static void installTranslations() noexcept
{
    // Install Qt translations
    QTranslator* qtTranslator = new QTranslator(qApp);
    qtTranslator->load("qt_" % QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    qApp->installTranslator(qtTranslator);

    // Install system language translations (all system languages defined in the system settings, in the defined order)
    QTranslator* systemTranslator = new QTranslator(qApp);
    systemTranslator->load(QLocale::system(), "librepcb", "_", ":/i18n/");
    qApp->installTranslator(systemTranslator);

    // Install language translations (like "de" for German)
    QTranslator* appTranslator1 = new QTranslator(qApp);
    appTranslator1->load("librepcb_" % QLocale::system().name().split("_").at(0), ":/i18n");
    qApp->installTranslator(appTranslator1);

    // Install language/country translations (like "de_ch" for German/Switzerland)
    QTranslator* appTranslator2 = new QTranslator(qApp);
    appTranslator2->load("librepcb_" % QLocale::system().name(), ":/i18n");
    qApp->installTranslator(appTranslator2);
}

/*****************************************************************************************
 *  init3rdPartyLibs()
 ****************************************************************************************/

static void init3rdPartyLibs() noexcept
{

}

/*****************************************************************************************
 *  cleanup3rdPartyLibs()
 ****************************************************************************************/

static void cleanup3rdPartyLibs() noexcept
{

}

/*****************************************************************************************
 *  determineWorkspacePath()
 ****************************************************************************************/

static FilePath determineWorkspacePath() noexcept
{
    FilePath wsPath(Workspace::getMostRecentlyUsedWorkspacePath());
    if (!Workspace::isValidWorkspacePath(wsPath)) {
        FirstRunWizard wizard;
        if (wizard.exec() == QDialog::Accepted) {
            wsPath = wizard.getWorkspaceFilePath();
            if (wizard.getCreateNewWorkspace()) {
                if (!Workspace::createNewWorkspace(wsPath)) {
                    QMessageBox::critical(0, Application::translate("Workspace",
                        "Error"), Application::translate("Workspace", "Could not "
                        "create the workspace directory. Check file permissions."));
                    return FilePath(); // TODO: Show the wizard again instead of closing the application
                }
            }
            Workspace::setMostRecentlyUsedWorkspacePath(wsPath);
        } else {
            return FilePath(); // abort
        }
    }
    return wsPath;
}

/*****************************************************************************************
 *  openWorkspace()
 ****************************************************************************************/

static int openWorkspace(const FilePath& path) noexcept
{
    try
    {
        Workspace ws(path);   // The Workspace constructor can throw an exception
        ControlPanel p(ws);
        p.show();

        return appExec();
    }
    catch (UserCanceled& e)
    {
        return 0;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(0, Application::translate("Workspace",
            "Cannot open the workspace"), QString(Application::translate(
            "Workspace", "The workspace \"%1\" cannot be opened: %2"))
            .arg(path.toNative(), e.getUserMsg()));
        return 0;
    }
}

/*****************************************************************************************
 *  appExec()
 ****************************************************************************************/

static int appExec() noexcept
{
    // please note that we shouldn't show a dialog or message box in the catch() blocks!
    // from http://qt-project.org/doc/qt-5/exceptionsafety.html:
    //      "After an exception is thrown, the connection to the windowing server might
    //      already be closed. It is not safe to call a GUI related function after
    //      catching an exception."
    try
    {
        return Application::exec();
    }
    catch (std::exception& e)
    {
        qFatal("UNCAUGHT EXCEPTION: %s --- PROGRAM EXITED", e.what());
    }
    catch (...)
    {
        qFatal("UNCAUGHT EXCEPTION --- PROGRAM EXITED");
    }

    return -1;
}
