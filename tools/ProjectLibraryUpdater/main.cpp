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
#include <librepcbcommon/application.h>
#include "mainwindow.h"

/*****************************************************************************************
 *  main()
 ****************************************************************************************/

int main(int argc, char* argv[])
{
    librepcb::Application app(argc, argv);

    QCoreApplication::setOrganizationName("LibrePCB");
    QCoreApplication::setApplicationName("ProjectLibraryUpdater");

    MainWindow w;
    w.show();

    return QApplication::exec();
}
