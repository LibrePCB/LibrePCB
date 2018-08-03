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
#include <librepcb/common/application.h>

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

TEST(ApplicationTest, testAppVersion)
{
    // read application version and check validity
    Version v = qApp->getAppVersion();
    EXPECT_GE(Version::fromString("0.1"), v);

    // compare with QApplication version
    Version v1 = Version::fromString(qApp->applicationVersion());
    EXPECT_GE(Version::fromString("0.1"), v1);
    EXPECT_EQ(v, v1);
}

TEST(ApplicationTest, testFileFormatVersion)
{
    // check minimum
    EXPECT_GE(Version::fromString("0.1"), qApp->getFileFormatVersion());

    // it can't be greater then the application version
    EXPECT_LE(qApp->getFileFormatVersion(), qApp->getAppVersion());
}

TEST(ApplicationTest, testGetResourcesDir)
{
    // check if the resources directory is valid, exists and is not empty
    EXPECT_TRUE(qApp->getResourcesDir().isValid());
    EXPECT_TRUE(qApp->getResourcesDir().isExistingDir());
    EXPECT_FALSE(qApp->getResourcesDir().isEmptyDir());

    // as the tests can't be installed, the resources must be located in the repository root
    FilePath repoRoot = qApp->getResourcesDir().getParentDir().getParentDir();
    EXPECT_TRUE(repoRoot.getPathTo("librepcb.pro").isExistingFile());
}

TEST(ApplicationTest, testGetResourcesFilePath)
{
    FilePath dir = qApp->getResourcesDir();
    EXPECT_EQ(dir, qApp->getResourcesFilePath(""));
    EXPECT_EQ(dir, qApp->getResourcesFilePath(QString()));
    EXPECT_EQ(dir.getPathTo("foo"), qApp->getResourcesFilePath("foo"));
    EXPECT_EQ(dir.getPathTo("foo/bar.ext"), qApp->getResourcesFilePath("foo/bar.ext"));
}

TEST(ApplicationTest, testExistenceOfResourceFiles)
{
    EXPECT_TRUE(qApp->getResourcesDir().isExistingDir());
    EXPECT_TRUE(qApp->getResourcesFilePath("README.md").isExistingFile());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
