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
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ProjectTest : public ::testing::Test {
protected:
  FilePath mProjectDir;
  FilePath mProjectFile;
  FilePath mLogsDir;

  ProjectTest() {
    // the whitespaces in the path are there to make the test even stronger ;)
    mProjectDir = FilePath::getRandomTempPath().getPathTo("test project dir");
    mProjectFile = mProjectDir.getPathTo("test project.lpp");
    mLogsDir = mProjectDir.getPathTo("logs");
  }

  virtual ~ProjectTest() {
    QDir(mProjectDir.getParentDir().toStr()).removeRecursively();
  }

  std::unique_ptr<TransactionalDirectory> createDir(
      bool writable = true) const noexcept {
    return std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
        TransactionalFileSystem::open(mProjectDir, writable)));
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ProjectTest, testUpgradeV01) {
  // Copy project into temporary directory.
  const FilePath src(TEST_DATA_DIR "/projects/v0.1");
  FileUtils::copyDirRecursively(src, mProjectDir);

  // Open/upgrade/close project.
  int schematicCount = -1;
  int boardCount = -1;
  ASSERT_TRUE(FileUtils::readFile(mProjectDir.getPathTo(".librepcb-project"))
                  .startsWith("0.1\n"));
  ASSERT_FALSE(mLogsDir.isExistingDir());
  {
    ProjectLoader loader;
    std::unique_ptr<Project> project =
        loader.open(createDir(), mProjectFile.getFilename());
    schematicCount = project->getSchematics().count();
    boardCount = project->getBoards().count();
    project->save();
    project->getDirectory().getFileSystem()->save();
  }

  // Check written files.
  EXPECT_TRUE(
      FileUtils::readFile(mProjectDir.getPathTo(".librepcb-project"))
          .startsWith(Application::getFileFormatVersion().toStr().toUtf8() +
                      "\n"));
  EXPECT_EQ(1,
            FileUtils::getFilesInDirectory(mLogsDir, {"*.html"}, false, true)
                .count());

  // Re-open project.
  {
    ProjectLoader loader;
    std::unique_ptr<Project> project =
        loader.open(createDir(), mProjectFile.getFilename());
    EXPECT_EQ(schematicCount, project->getSchematics().count());
    EXPECT_EQ(boardCount, project->getBoards().count());
  }
}

TEST_F(ProjectTest, testCreateCloseOpen) {
  QDateTime datetime = QDateTime::currentDateTime();

  // create new project
  std::unique_ptr<Project> project =
      Project::create(createDir(), mProjectFile.getFilename());
  EXPECT_EQ(mProjectFile, project->getFilepath());
  EXPECT_EQ(mProjectDir, project->getPath());
  EXPECT_EQ("Unnamed", project->getName());
  EXPECT_EQ("", project->getAuthor());
  EXPECT_EQ("v1", *project->getVersion());
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getCreated().toMSecsSinceEpoch(), 5000);
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getDateTime().toMSecsSinceEpoch(), 5000);
  EXPECT_EQ(0, project->getSchematics().count());
  EXPECT_EQ(0, project->getBoards().count());

  // save and close project
  project->save();
  project->getDirectory().getFileSystem()->save();
  project.reset();

  // check existence of files
  EXPECT_TRUE(mProjectDir.isExistingDir());
  EXPECT_FALSE(mProjectDir.isEmptyDir());
  EXPECT_TRUE(mProjectFile.isExistingFile());
  EXPECT_TRUE(mProjectDir.getPathTo(".librepcb-project").isExistingFile());
  EXPECT_TRUE(mProjectDir.getPathTo("project/metadata.lp").isExistingFile());
  EXPECT_TRUE(mProjectDir.getPathTo("project/settings.lp").isExistingFile());
  EXPECT_TRUE(mProjectDir.getPathTo("circuit/circuit.lp").isExistingFile());
  EXPECT_TRUE(mProjectDir.getPathTo("circuit/erc.lp").isExistingFile());

  // open project again
  ProjectLoader loader;
  project = loader.open(createDir(), mProjectFile.getFilename());
  EXPECT_EQ(mProjectFile, project->getFilepath());
  EXPECT_EQ(mProjectDir, project->getPath());
  EXPECT_EQ("Unnamed", project->getName());
  EXPECT_EQ("", project->getAuthor());
  EXPECT_EQ("v1", *project->getVersion());
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getCreated().toMSecsSinceEpoch(), 5000);
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getDateTime().toMSecsSinceEpoch(), 5000);
  EXPECT_EQ(0, project->getSchematics().count());
  EXPECT_EQ(0, project->getBoards().count());
}

TEST_F(ProjectTest, testSave) {
  // create new project
  std::unique_ptr<Project> project =
      Project::create(createDir(), mProjectFile.getFilename());

  // save project
  project->save();
  project->getDirectory().getFileSystem()->save();

  // close and re-open project
  {
    project.reset();
    ProjectLoader loader;
    project = loader.open(createDir(), mProjectFile.getFilename());
  }

  // save project
  project->save();
  project->getDirectory().getFileSystem()->save();

  // close and re-open project
  {
    project.reset();
    ProjectLoader loader;
    project = loader.open(createDir(), mProjectFile.getFilename());
  }
}

TEST_F(ProjectTest, testIfDateTimeIsUpdatedOnSave) {
  // create new project
  std::unique_ptr<Project> project =
      Project::create(createDir(), mProjectFile.getFilename());
  qint64 datetimeAfterCreating = project->getDateTime().toMSecsSinceEpoch();

  // check if datetime has not changed
  QThread::msleep(1000);
  EXPECT_EQ(datetimeAfterCreating, project->getDateTime().toMSecsSinceEpoch());

  // save project and verify that datetime has changed
  QThread::msleep(1000);
  project->save();
  qint64 datetimeAfterSaving = project->getDateTime().toMSecsSinceEpoch();
  EXPECT_NEAR(QDateTime::currentMSecsSinceEpoch(), datetimeAfterSaving,
              1000);  // +/- 1s
  EXPECT_NE(datetimeAfterCreating, datetimeAfterSaving);
}

TEST_F(ProjectTest, testSettersGetters) {
  // create new project
  std::unique_ptr<Project> project =
      Project::create(createDir(), mProjectFile.getFilename());

  // set properties
  ElementName name("test name 1234");
  QString author = "test author 1234";
  FileProofName version("test-version-12.34");
  project->setName(name);
  project->setAuthor(author);
  project->setVersion(version);

  // get properties
  EXPECT_EQ(name, project->getName());
  EXPECT_EQ(author, project->getAuthor());
  EXPECT_EQ(version, project->getVersion());

  // save project
  project->save();
  project->getDirectory().getFileSystem()->save();

  // close and re-open project (read-only)
  project.reset();
  ProjectLoader loader;
  project = loader.open(createDir(false), mProjectFile.getFilename());

  // get properties
  EXPECT_EQ(name, project->getName());
  EXPECT_EQ(author, project->getAuthor());
  EXPECT_EQ(version, project->getVersion());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
