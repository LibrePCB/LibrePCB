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
#include "units/all_length_units.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Application::Application(int& argc, char** argv) noexcept :
    QApplication(argc, argv)
{
    // register meta types
    qRegisterMetaType<FilePath>();
    qRegisterMetaType<Point>();

    // set application version
    mAppVersion = Version(APP_VERSION);
    QApplication::setApplicationVersion(mAppVersion.toPrettyStr(2));
    Q_ASSERT(mAppVersion.isValid());

    // set "git describe" version
    mGitVersion = QString(GIT_VERSION);
    if (mGitVersion.isEmpty()) {
        qWarning() << "Could not determine Git version. Check if Git is added to the PATH.";
    }

    // set and verify file format version
    mFileFormatVersion = Version(FILE_FORMAT_VERSION);
    Q_ASSERT(mFileFormatVersion.isValid());
    Q_ASSERT_X(mFileFormatVersion.isPrefixOf(mAppVersion),
               "Application::Application()",
               "The file format version is not a prefix of the application version. "
               "Please correct this in the file 'librepcbcommon.pro'.");

    // get the directory of the currently running executable
    FilePath executableDirPath(QApplication::applicationDirPath());
    Q_ASSERT(executableDirPath.isValid());

    // determine the path to the resources directory (e.g. /usr/share/librepcb)
    mResourcesDir = executableDirPath.getPathTo("../share/librepcb");
    Q_ASSERT(mResourcesDir.isValid());
}

Application::~Application() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

FilePath Application::getResourcesFilePath(const QString& filepath) const noexcept
{
    return mResourcesDir.getPathTo(filepath);
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

Application* Application::instance() noexcept
{
    Application* app = dynamic_cast<Application*>(QCoreApplication::instance());
    Q_ASSERT(app);
    return app;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
