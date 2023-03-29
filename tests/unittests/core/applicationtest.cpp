/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gtest/gtest.h>
#include <librepcb/core/application.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ApplicationTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(ApplicationTest, testGetVersion) {
  EXPECT_FALSE(Application::getVersion().isEmpty());
}

TEST(ApplicationTest, testGetFileFormatVersion) {
  EXPECT_GE(Application::getFileFormatVersion(), Version::fromString("0.1"));
}

TEST(ApplicationTest, testGetResourcesDir) {
  // check if the resources directory is valid, exists and is not empty
  EXPECT_TRUE(Application::getResourcesDir().isValid());
  EXPECT_TRUE(Application::getResourcesDir().isExistingDir());
  EXPECT_FALSE(Application::getResourcesDir().isEmptyDir());

  // as the tests can't be installed, the resources must be located in the
  // repository root
  FilePath repoRoot =
      Application::getResourcesDir().getParentDir().getParentDir();
  EXPECT_TRUE(repoRoot.getPathTo("LICENSE.txt").isExistingFile());
  EXPECT_TRUE(repoRoot.getPathTo("CMakeLists.txt").isExistingFile());
}

TEST(ApplicationTest, testExistenceOfResourceFiles) {
  EXPECT_TRUE(Application::getResourcesDir().isExistingDir());
  EXPECT_TRUE(
      Application::getResourcesDir().getPathTo("README.md").isExistingFile());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
