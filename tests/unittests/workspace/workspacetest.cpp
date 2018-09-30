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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gtest/gtest.h>
#include <librepcb/common/application.h>
#include <librepcb/common/fileio/smartversionfile.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class WorkspaceTest : public ::testing::Test {
protected:
  FilePath mWsDir;
  FilePath mVersionFile;
  FilePath mProjectsPath;
  FilePath mMetadataPath;
  FilePath mLibrariesPath;

  WorkspaceTest() {
    // the whitespaces in the path are there to make the test even stronger ;)
    mWsDir = FilePath::getRandomTempPath().getPathTo("test workspace dir");
    mVersionFile  = mWsDir.getPathTo(".librepcb-workspace");
    mProjectsPath = mWsDir.getPathTo("projects");
    mMetadataPath =
        mWsDir.getPathTo("v" % qApp->getFileFormatVersion().toStr());
    mLibrariesPath = mMetadataPath.getPathTo("libraries");
  }

  virtual ~WorkspaceTest() {
    QDir(mWsDir.getParentDir().toStr()).removeRecursively();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(WorkspaceTest, testCreateOpenClose) {
  // create new workspace
  EXPECT_NO_THROW(Workspace::createNewWorkspace(mWsDir));

  // check existence of workspace directory and version file
  EXPECT_TRUE(mWsDir.isExistingDir());
  EXPECT_TRUE(mVersionFile.isExistingFile());

  // open/close workspace
  {
    Workspace ws(mWsDir);
    EXPECT_EQ(mWsDir, ws.getPath());
    EXPECT_EQ(mProjectsPath, ws.getProjectsPath());
    EXPECT_EQ(mMetadataPath, ws.getMetadataPath());
    EXPECT_EQ(mLibrariesPath, ws.getLibrariesPath());
  }

  // open/close workspace again
  { Workspace ws(mWsDir); }
}

TEST_F(WorkspaceTest, testOpenNonExistingWorkspace) {
  EXPECT_THROW(Workspace ws(mWsDir), Exception);
}

TEST_F(WorkspaceTest, testOpenIncompatibleWorkspaceVersion) {
  Workspace::createNewWorkspace(mWsDir);
  SmartVersionFile versionFile(mVersionFile, false, false);
  EXPECT_EQ(Workspace::FILE_FORMAT_VERSION(), versionFile.getVersion());
  versionFile.setVersion(
      Version::fromString("0.0.1"));  // version 0.0.1 will never exist
  versionFile.save(true);
  EXPECT_THROW(Workspace ws(mWsDir), Exception);
}

TEST_F(WorkspaceTest, testIfOpeningWorkspaceMultipleTimesFails) {
  Workspace::createNewWorkspace(mWsDir);
  Workspace w1s(mWsDir);
  EXPECT_THROW(Workspace ws2(mWsDir), Exception);
}

TEST_F(WorkspaceTest, testIsValidWorkspacePath) {
  EXPECT_FALSE(Workspace::isValidWorkspacePath(mWsDir));
  Workspace::createNewWorkspace(mWsDir);
  EXPECT_TRUE(Workspace::isValidWorkspacePath(mWsDir));
}

TEST_F(WorkspaceTest, testGetFileFormatVersionsOfWorkspace) {
  EXPECT_TRUE(Workspace::getFileFormatVersionsOfWorkspace(mWsDir).isEmpty());
  Workspace::createNewWorkspace(mWsDir);
  EXPECT_TRUE(Workspace::getFileFormatVersionsOfWorkspace(mWsDir).isEmpty());
  Workspace ws(mWsDir);
  EXPECT_EQ(QList<Version>{qApp->getFileFormatVersion()},
            Workspace::getFileFormatVersionsOfWorkspace(mWsDir));
}

TEST_F(WorkspaceTest, testGetHighestFileFormatVersionOfWorkspace) {
  EXPECT_FALSE(
      Workspace::getHighestFileFormatVersionOfWorkspace(mWsDir).has_value());
  Workspace::createNewWorkspace(mWsDir);
  EXPECT_FALSE(
      Workspace::getHighestFileFormatVersionOfWorkspace(mWsDir).has_value());
  Workspace ws(mWsDir);
  EXPECT_EQ(qApp->getFileFormatVersion(),
            Workspace::getHighestFileFormatVersionOfWorkspace(mWsDir));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace workspace
}  // namespace librepcb
