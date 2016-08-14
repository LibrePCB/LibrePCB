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
#include "application.h"
#include "exceptions.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Application::Application(int& argc, char** argv) :
    QApplication(argc, argv)
{
}

Application::~Application()
{
}

/*****************************************************************************************
 *  Reimplemented from QApplication
 ****************************************************************************************/

bool Application::notify(QObject* receiver, QEvent* e)
{
    try
    {
        return QApplication::notify(receiver, e);
    }
    catch (...)
    {
        qCritical() << "Exception caught in Application::notify()!";
    }
    return false;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

void Application::setApplicationVersion(const Version& version) noexcept
{
    QApplication::setApplicationVersion(version.toStr());
}

Version Application::applicationVersion() noexcept
{
    Version version(QApplication::applicationVersion());
    Q_ASSERT(version.isValid());
    return version;
}

bool Application::isRunningFromInstalledExecutable() noexcept
{
    // get the installation prefix directory
    FilePath installationPrefixDir(INSTALLATION_PREFIX);
    Q_ASSERT(installationPrefixDir.isValid());

    // get the directory of the currently running executable
    FilePath executableFilePath(qApp->applicationFilePath());
    Q_ASSERT(executableFilePath.isValid());

    // check if the running executable is located in the installation prefix directory
    return executableFilePath.isLocatedInDir(installationPrefixDir);
}

FilePath Application::getResourcesDir() noexcept
{
    if (isRunningFromInstalledExecutable()) {
        // the application is installed on the system -> use installed ressources
        return FilePath(INSTALLED_RESOURCES_DIR);
    } else {
        // the application is not installed on the system -> use local resources
        return FilePath(LOCAL_RESOURCES_DIR);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
