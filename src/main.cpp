/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <QApplication>
#include <QTranslator>
#include "workspace/workspace.h"
#include "workspace/workspacechooserdialog.h"

/*****************************************************************************************
 *  main()
 ****************************************************************************************/

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load("qt_" % QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    QTranslator appTranslator;
    appTranslator.load("eda4u_" % QLocale::system().name(), ":/i18n");
    app.installTranslator(&appTranslator);

    QCoreApplication::setOrganizationName("EDA4U");
    //QCoreApplication::setOrganizationDomain(""); ///< @todo
    QCoreApplication::setApplicationName("EDA4U");
    QCoreApplication::setApplicationVersion("0.0.1");

    QGuiApplication::setQuitOnLastWindowClosed(false);

    // Initialization finished, open the workspace...

    QDir workspaceDir;
    workspaceDir.setPath(Workspace::getMostRecentlyUsedWorkspacePath());

    if ((!workspaceDir.exists()) || (!Workspace::isValidWorkspaceDir(workspaceDir)))
    {
        WorkspaceChooserDialog dialog;

        if (!dialog.exec())
            return 0; // no workspace was choosed

        workspaceDir = dialog.getChoosedWorkspaceDir();
    }

    Workspace ws(workspaceDir);
    ws.showControlPanel();

    return app.exec();
}
