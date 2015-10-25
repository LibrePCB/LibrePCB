/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
 *  app.exec()
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
    catch (Exception& e)
    {
        qFatal("UNCAUGHT EXCEPTION: %s --- PROGRAM EXITED", qPrintable(e.getUserMsg()));
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

/*****************************************************************************************
 *  main()
 ****************************************************************************************/

int main(int argc, char* argv[])
{
    Application app(argc, argv);

    // Set the organization / application names must be done very early because some other
    // classes will use these values (for example QSettings, Debug (for the file logging path))!
    Application::setOrganizationName("LibrePCB");
    Application::setOrganizationDomain("librepcb.org");
#ifdef GIT_BRANCH
    Application::setApplicationName(QString("LibrePCB_git-%1").arg(GIT_BRANCH));
#else
    Application::setApplicationName("LibrePCB");
#endif
    Application::setApplicationVersion(Version(QString("%1.%2.%3").arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH)));


    Debug::instance(); // this creates the Debug object and installs the message handler.


    // Install Qt translations
    QTranslator qtTranslator;
    qtTranslator.load("qt_" % QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    // Install system language translations (all system languages defined in the system settings, in the defined order)
    QTranslator systemTranslator;
    systemTranslator.load(QLocale::system(), "librepcb", "_", ":/i18n/");
    app.installTranslator(&systemTranslator);

    // Install language translations (like "de" for German)
    QTranslator appTranslator1;
    appTranslator1.load("librepcb_" % QLocale::system().name().split("_").at(0), ":/i18n");
    app.installTranslator(&appTranslator1);

    // Install language/country translations (like "de_ch" for German/Switzerland)
    QTranslator appTranslator2;
    appTranslator2.load("librepcb_" % QLocale::system().name(), ":/i18n");
    app.installTranslator(&appTranslator2);


    Application::setQuitOnLastWindowClosed(false);


    // this is to remove the ugly frames around widgets in all status bars...
    // (from http://www.qtcentre.org/threads/1904)
    app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");



    // Initialization finished, open the workspace (or show the first run wizard)...
    FilePath wsPath(Workspace::getMostRecentlyUsedWorkspacePath());
    if (!Workspace::isValidWorkspacePath(wsPath))
    {
        FirstRunWizard wizard;
        if (wizard.exec() != QDialog::Accepted) return 0;
        wsPath = wizard.getWorkspaceFilePath();
        if (wizard.getCreateNewWorkspace())
        {
            if (!Workspace::createNewWorkspace(wsPath))
            {
                QMessageBox::critical(0, Application::translate("Workspace",
                    "Error"), Application::translate("Workspace", "Could not "
                    "create the workspace directory. Check file permissions."));
                return 0; // TODO: Show the wizard again instead of closing the application
            }
        }
        Workspace::setMostRecentlyUsedWorkspacePath(wsPath);
    }

    try
    {
        Workspace ws(wsPath);   // The Workspace constructor can throw an exception.
        ControlPanel p(ws);
        p.show();

        return appExec();
    }
    catch (UserCanceled& e)
    {
        return 0; // quit the application
    }
    catch (Exception& e)
    {
        QMessageBox::critical(0, Application::translate("Workspace",
            "Cannot open the workspace"), QString(Application::translate(
            "Workspace", "The workspace \"%1\" cannot be opened: %2"))
            .arg(wsPath.toNative(), e.getUserMsg()));
        return 0; // quit the application
    }

    return 0;
}
