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
#include <gtest/gtest.h>
#include <librepcbcommon/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class ApplicationTest : public ::testing::Test
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST(ApplicationTest, testApplicationVersion)
{
    // read application version and check validity
    Version v = Application::applicationVersion();
    EXPECT_TRUE(v.isValid());

    // compare with QApplication version
    Version v1(qApp->applicationVersion());
    EXPECT_TRUE(v1.isValid());
    EXPECT_EQ(v, v1);

    // compare with defines
    Version v2(QString("%1.%2.%3").arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH));
    EXPECT_TRUE(v2.isValid());
    EXPECT_EQ(v, v2);
}

TEST(ApplicationTest, testMajorVersion)
{
    EXPECT_EQ(APP_VERSION_MAJOR, Application::majorVersion());
}

TEST(ApplicationTest, testIsRunningFromInstalledExecutable)
{
    // as there is no "make install" available for the unit tests, it can't be installed ;)
    EXPECT_FALSE(Application::isRunningFromInstalledExecutable());
}

TEST(ApplicationTest, testGetResourcesDir)
{
    // as the tests can't be installed, the resources must be located in LOCAL_RESOURCES_DIR
    EXPECT_EQ(Application::getResourcesDir(), FilePath(LOCAL_RESOURCES_DIR));

    // check if the resources directory is valid, exists and is not empty
    EXPECT_TRUE(Application::getResourcesDir().isValid());
    EXPECT_TRUE(Application::getResourcesDir().isExistingDir());
    EXPECT_FALSE(Application::getResourcesDir().isEmptyDir());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
