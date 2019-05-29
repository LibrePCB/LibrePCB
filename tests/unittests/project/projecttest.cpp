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
#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/project/metadata/projectmetadata.h>
#include <librepcb/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ProjectTest : public ::testing::Test {
protected:
  FilePath mProjectDir;
  FilePath mProjectFile;

  ProjectTest() {
    // the whitespaces in the path are there to make the test even stronger ;)
    mProjectDir  = FilePath::getRandomTempPath().getPathTo("test project dir");
    mProjectFile = mProjectDir.getPathTo("test project.lpp");
  }

  virtual ~ProjectTest() {
    QDir(mProjectDir.getParentDir().toStr()).removeRecursively();
  }

  std::unique_ptr<TransactionalDirectory> createDir(bool writable = true) const
      noexcept {
    return std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
        TransactionalFileSystem::open(mProjectDir, writable)));
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ProjectTest, testCreateCloseOpen) {
  QDateTime datetime = QDateTime::currentDateTime();

  // create new project
  QScopedPointer<Project> project(
      Project::create(createDir(), mProjectFile.getFilename()));
  EXPECT_EQ(mProjectFile, project->getFilepath());
  EXPECT_EQ(mProjectDir, project->getPath());
  EXPECT_EQ(mProjectFile.getCompleteBasename(),
            project->getMetadata().getName());
  EXPECT_EQ("Unknown", project->getMetadata().getAuthor());
  EXPECT_EQ(QString("v1"), project->getMetadata().getVersion());
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getMetadata().getCreated().toMSecsSinceEpoch(), 5000);
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getMetadata().getLastModified().toMSecsSinceEpoch(),
              5000);
  EXPECT_EQ(0, project->getSchematics().count());
  EXPECT_EQ(0, project->getBoards().count());

  // save and close project
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
  project.reset(new Project(createDir(), mProjectFile.getFilename()));
  EXPECT_EQ(mProjectFile, project->getFilepath());
  EXPECT_EQ(mProjectDir, project->getPath());
  EXPECT_EQ(mProjectFile.getCompleteBasename(),
            project->getMetadata().getName());
  EXPECT_EQ("Unknown", project->getMetadata().getAuthor());
  EXPECT_EQ(QString("v1"), project->getMetadata().getVersion());
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getMetadata().getCreated().toMSecsSinceEpoch(), 5000);
  EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
              project->getMetadata().getLastModified().toMSecsSinceEpoch(),
              5000);
  EXPECT_EQ(0, project->getSchematics().count());
  EXPECT_EQ(0, project->getBoards().count());
}

TEST_F(ProjectTest, testSave) {
  // create new project
  QScopedPointer<Project> project(
      Project::create(createDir(), mProjectFile.getFilename()));

  // save project
  project->save();
  project->getDirectory().getFileSystem()->save();

  // close and re-open project
  project.reset();
  project.reset(new Project(createDir(), mProjectFile.getFilename()));

  // save project
  project->save();
  project->getDirectory().getFileSystem()->save();

  // close and re-open project
  project.reset();
  project.reset(new Project(createDir(), mProjectFile.getFilename()));
}

TEST_F(ProjectTest, testIfLastModifiedDateTimeIsUpdatedOnSave) {
  // create new project
  QScopedPointer<Project> project(
      Project::create(createDir(), mProjectFile.getFilename()));
  qint64 datetimeAfterCreating =
      project->getMetadata().getLastModified().toMSecsSinceEpoch();

  // check if datetime has not changed
  QThread::msleep(1000);
  EXPECT_EQ(datetimeAfterCreating,
            project->getMetadata().getLastModified().toMSecsSinceEpoch());

  // save project and verify that datetime has changed
  QThread::msleep(1000);
  project->save();
  qint64 datetimeAfterSaving =
      project->getMetadata().getLastModified().toMSecsSinceEpoch();
  EXPECT_NEAR(QDateTime::currentMSecsSinceEpoch(), datetimeAfterSaving,
              1000);  // +/- 1s
  EXPECT_NE(datetimeAfterCreating, datetimeAfterSaving);
}

TEST_F(ProjectTest, testSettersGetters) {
  // create new project
  QScopedPointer<Project> project(
      Project::create(createDir(), mProjectFile.getFilename()));

  // set properties
  ElementName name("test name 1234");
  QString     author  = "test author 1234";
  QString     version = "test version 1234";
  project->getMetadata().setName(name);
  project->getMetadata().setAuthor(author);
  project->getMetadata().setVersion(version);

  // get properties
  EXPECT_EQ(name, project->getMetadata().getName());
  EXPECT_EQ(author, project->getMetadata().getAuthor());
  EXPECT_EQ(version, project->getMetadata().getVersion());

  // save project
  project->save();
  project->getDirectory().getFileSystem()->save();

  // close and re-open project (read-only)
  project.reset();
  project.reset(new Project(createDir(false), mProjectFile.getFilename()));

  // get properties
  EXPECT_EQ(name, project->getMetadata().getName());
  EXPECT_EQ(author, project->getMetadata().getAuthor());
  EXPECT_EQ(version, project->getMetadata().getVersion());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace project
}  // namespace librepcb
