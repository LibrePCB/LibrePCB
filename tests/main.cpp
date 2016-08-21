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
#include <gmock/gmock.h>
#include <librepcbcommon/application.h>
#include <librepcbcommon/debug.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
using namespace librepcb;

/*****************************************************************************************
 *  The Unit Testing Program
 ****************************************************************************************/

int main(int argc, char *argv[])
{
    // many classes rely on a QApplication instance, so we create it here
    Application app(argc, argv);
    Application::setOrganizationName("LibrePCB");
    Application::setOrganizationDomain("librepcb.org");
    Application::setApplicationName("LibrePCB-UnitTests");
    Application::setApplicationVersion(Version(QString("%1.%2.%3").arg(APP_VERSION_MAJOR)
                                       .arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH)));

    // disable the whole debug output (we want only the output from gtest)
    Debug::instance()->setDebugLevelLogFile(Debug::DebugLevel_t::Nothing);
    Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::Nothing);

    // init gmock and run all tests
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
