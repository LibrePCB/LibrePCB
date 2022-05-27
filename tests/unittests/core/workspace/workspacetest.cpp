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
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/versionfile.h>
#include <librepcb/core/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class WorkspaceTest : public ::testing::Test {
protected:
  FilePath mWsDir;
  FilePath mVersionFile;
  FilePath mProjectsPath;
  FilePath mDataPath;
  FilePath mLibrariesPath;

  WorkspaceTest() {
    // the whitespaces in the path are there to make the test even stronger ;)
    mWsDir = FilePath::getRandomTempPath().getPathTo("test workspace dir");
    mVersionFile = mWsDir.getPathTo(".librepcb-workspace");
    mProjectsPath = mWsDir.getPathTo("projects");
    mDataPath = mWsDir.getPathTo("data");
    mLibrariesPath = mDataPath.getPathTo("libraries");
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
    Workspace ws(mWsDir, "data");
    EXPECT_EQ(mWsDir, ws.getPath());
    EXPECT_EQ(mProjectsPath, ws.getProjectsPath());
    EXPECT_EQ(mDataPath, ws.getDataPath());
    EXPECT_EQ(mLibrariesPath, ws.getLibrariesPath());
  }

  // open/close workspace again
  { Workspace ws(mWsDir, "data"); }
}

TEST_F(WorkspaceTest, testOpenNonExistingWorkspace) {
  EXPECT_THROW(Workspace ws(mWsDir, "data"), Exception);
}

TEST_F(WorkspaceTest, testOpenIncompatibleWorkspaceVersion) {
  Workspace::createNewWorkspace(mWsDir);
  VersionFile versionFile =
      VersionFile::fromByteArray(FileUtils::readFile(mVersionFile));
  EXPECT_EQ(Workspace::FILE_FORMAT_VERSION(), versionFile.getVersion());
  versionFile.setVersion(
      Version::fromString("0.0.1"));  // version 0.0.1 will never exist
  FileUtils::writeFile(mVersionFile, versionFile.toByteArray());
  EXPECT_THROW(Workspace ws(mWsDir, "data"), Exception);
}

TEST_F(WorkspaceTest, testIfOpeningWorkspaceMultipleTimesFails) {
  Workspace::createNewWorkspace(mWsDir);
  Workspace w1s(mWsDir, "data");
  EXPECT_THROW(Workspace ws2(mWsDir, "data"), Exception);
}

TEST_F(WorkspaceTest, testCheckCOmpatibility) {
  {
    QString errorMsg;
    EXPECT_FALSE(Workspace::checkCompatibility(mWsDir));
    EXPECT_FALSE(Workspace::checkCompatibility(mWsDir, &errorMsg));
    EXPECT_FALSE(errorMsg.isEmpty());
  }
  Workspace::createNewWorkspace(mWsDir);
  {
    QString errorMsg;
    EXPECT_TRUE(Workspace::checkCompatibility(mWsDir));
    EXPECT_TRUE(Workspace::checkCompatibility(mWsDir, &errorMsg));
    EXPECT_TRUE(errorMsg.isEmpty());
  }
}

TEST_F(WorkspaceTest, testFindDataDirectories) {
  EXPECT_TRUE(Workspace::findDataDirectories(mWsDir).isEmpty());
  Workspace::createNewWorkspace(mWsDir);
  EXPECT_TRUE(Workspace::findDataDirectories(mWsDir).isEmpty());
  { Workspace ws(mWsDir, "data"); }
  EXPECT_EQ((QMap<QString, Version>{{"data", qApp->getFileFormatVersion()}}),
            Workspace::findDataDirectories(mWsDir));
  { Workspace ws(mWsDir, "v0.1"); }
  EXPECT_EQ((QMap<QString, Version>{{"data", qApp->getFileFormatVersion()},
                                    {"v0.1", Version::fromString("0.1")}}),
            Workspace::findDataDirectories(mWsDir));
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryEmpty) {
  QMap<QString, Version> dataDirs;
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("", copyFromDir.toStdString());
  EXPECT_EQ("", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOnlyOlderVersion) {
  QMap<QString, Version> dataDirs = {
      {"v0.0.1", Version::fromString("0.0.1")},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("v0.0.1", copyFromDir.toStdString());
  EXPECT_EQ("data", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOnlyCurrentVersion) {
  QString versionedDirName = "v" % qApp->getFileFormatVersion().toStr();
  QMap<QString, Version> dataDirs = {
      {versionedDirName, qApp->getFileFormatVersion()},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ(versionedDirName.toStdString(), dataDir.toStdString());
  EXPECT_EQ("", copyFromDir.toStdString());
  EXPECT_EQ("", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOnlyNewerVersion) {
  QMap<QString, Version> dataDirs = {
      {"v999", Version::fromString("999")},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("", copyFromDir.toStdString());
  EXPECT_EQ("", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOlderAndNewerVersions) {
  QMap<QString, Version> dataDirs = {
      {"v0.0.1", Version::fromString("0.0.1")},
      {"v0.0.2", Version::fromString("0.0.2")},
      {"v999", Version::fromString("999")},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("v0.0.2", copyFromDir.toStdString());
  EXPECT_EQ("data", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOnlyOlderVersionInData) {
  QMap<QString, Version> dataDirs = {
      {"data", Version::fromString("0.0.1")},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("data", copyFromDir.toStdString());
  EXPECT_EQ("v0.0.1", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOnlyCurrentVersionInData) {
  QMap<QString, Version> dataDirs = {
      {"data", qApp->getFileFormatVersion()},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("", copyFromDir.toStdString());
  EXPECT_EQ("", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOnlyNewerVersionInData) {
  QString versionedDirName = "v" % qApp->getFileFormatVersion().toStr();
  QMap<QString, Version> dataDirs = {
      {"data", Version::fromString("999")},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ(versionedDirName.toStdString(), dataDir.toStdString());
  EXPECT_EQ("", copyFromDir.toStdString());
  EXPECT_EQ("", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOlderVersionInDataWithBackups) {
  QMap<QString, Version> dataDirs = {
      {"v0.0.1", Version::fromString("0.0.1")},
      {"v0.0.2", Version::fromString("0.0.2")},
      {"data", Version::fromString("0.0.3")},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("data", copyFromDir.toStdString());
  EXPECT_EQ("v0.0.3", copyToDir.toStdString());
}

TEST_F(WorkspaceTest,
       testDetermineDataDirectoryCurrentVersionInDataWithBackups) {
  QMap<QString, Version> dataDirs = {
      {"v0.0.1", Version::fromString("0.0.1")},
      {"v0.0.2", Version::fromString("0.0.2")},
      {"data", qApp->getFileFormatVersion()},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("", copyFromDir.toStdString());
  EXPECT_EQ("", copyToDir.toStdString());
}

TEST_F(WorkspaceTest, testDetermineDataDirectoryOlderVersionInDataAndBackup) {
  QMap<QString, Version> dataDirs = {
      {"v0.0.1", Version::fromString("0.0.1")},
      {"data", Version::fromString("0.0.1")},
  };
  QString copyFromDir = "foo";
  QString copyToDir = "foo";
  QString dataDir =
      Workspace::determineDataDirectory(dataDirs, copyFromDir, copyToDir);
  EXPECT_EQ("data", dataDir.toStdString());
  EXPECT_EQ("", copyFromDir.toStdString());
  EXPECT_EQ("", copyToDir.toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
