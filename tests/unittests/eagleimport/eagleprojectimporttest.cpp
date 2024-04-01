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
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/eagleimport/eagleprojectimport.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class EagleProjectImportTest : public ::testing::Test {
protected:
  FilePath mTmpDir;

  EagleProjectImportTest() : mTmpDir(FilePath::getRandomTempPath()) {}

  ~EagleProjectImportTest() { QDir(mTmpDir.toStr()).removeRecursively(); }

  FilePath getSch(const QString& fp) const noexcept {
    return FilePath(TEST_DATA_DIR "/unittests/eagleimport/" % fp);
  }

  FilePath getBrd(const QString& fp) const noexcept {
    return FilePath(TEST_DATA_DIR "/unittests/eagleimport/" % fp);
  }

  std::unique_ptr<TransactionalDirectory> getProjectDir() const {
    return std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(TransactionalFileSystem::openRW(mTmpDir)));
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(EagleProjectImportTest, testImportOnlySchematic) {
  EagleProjectImport import;
  EXPECT_FALSE(import.isReady());

  const QStringList msgs = import.open(getSch("testproject.sch"), FilePath());
  EXPECT_TRUE(import.isReady());
  EXPECT_EQ("testproject", import.getProjectName().toStdString());
  EXPECT_EQ("Project contains buses which are not supported yet!",
            msgs.join(";").toStdString());

  {
    // Populate and save project.
    std::unique_ptr<Project> project =
        Project::create(getProjectDir(), "test.lpp");
    import.import(*project);
    project->save();
    project->getDirectory().getFileSystem()->save();
  }

  {
    // Open project again to see if there's no problem with it.
    ProjectLoader loader;
    std::unique_ptr<Project> project = loader.open(getProjectDir(), "test.lpp");
    EXPECT_EQ(1, project->getSchematics().count());
    EXPECT_EQ(0, project->getBoards().count());
  }
}

TEST_F(EagleProjectImportTest, testImportWithBoard) {
  EagleProjectImport import;
  EXPECT_FALSE(import.isReady());

  const QStringList msgs =
      import.open(getSch("testproject.sch"), getBrd("testproject.brd"));
  EXPECT_TRUE(import.isReady());
  EXPECT_EQ("testproject", import.getProjectName().toStdString());
  EXPECT_EQ("Project contains buses which are not supported yet!",
            msgs.join(";").toStdString());

  {
    // Populate and save project.
    std::unique_ptr<Project> project =
        Project::create(getProjectDir(), "test.lpp");
    import.import(*project);
    project->save();
    project->getDirectory().getFileSystem()->save();
  }

  {
    // Open project again to see if there's no problem with it.
    ProjectLoader loader;
    std::unique_ptr<Project> project = loader.open(getProjectDir(), "test.lpp");
    EXPECT_EQ(1, project->getSchematics().count());
    EXPECT_EQ(1, project->getBoards().count());
  }
}

// This project has strange embedded libraries which shall be tested.
TEST_F(EagleProjectImportTest, testArduinoMicro) {
  EagleProjectImport import;
  EXPECT_FALSE(import.isReady());

  const QStringList msgs = import.open(getSch("arduino-micro/Micro_Rev1j.sch"),
                                       getBrd("arduino-micro/Micro_Rev1j.brd"));
  EXPECT_TRUE(import.isReady());
  EXPECT_EQ("Micro_Rev1j", import.getProjectName().toStdString());
  EXPECT_EQ("", msgs.join(";").toStdString());

  {
    // Populate and save project.
    std::unique_ptr<Project> project =
        Project::create(getProjectDir(), "test.lpp");
    import.import(*project);
    project->save();
    project->getDirectory().getFileSystem()->save();
  }

  {
    // Open project again to see if there's no problem with it.
    ProjectLoader loader;
    std::unique_ptr<Project> project = loader.open(getProjectDir(), "test.lpp");
    EXPECT_EQ(1, project->getSchematics().count());
    EXPECT_EQ(1, project->getBoards().count());
  }
}

TEST_F(EagleProjectImportTest, testNodino) {
  EagleProjectImport import;
  EXPECT_FALSE(import.isReady());

  const QStringList msgs = import.open(getSch("nodino-rc7/Nodino-RC7.sch"),
                                       getBrd("nodino-rc7/Nodino-RC7.brd"));
  EXPECT_TRUE(import.isReady());
  EXPECT_EQ("Nodino-RC7", import.getProjectName().toStdString());
  EXPECT_EQ("", msgs.join(";").toStdString());

  {
    // Populate and save project.
    std::unique_ptr<Project> project =
        Project::create(getProjectDir(), "test.lpp");
    import.import(*project);
    project->save();
    project->getDirectory().getFileSystem()->save();
  }

  {
    // Open project again to see if there's no problem with it.
    ProjectLoader loader;
    std::unique_ptr<Project> project = loader.open(getProjectDir(), "test.lpp");
    EXPECT_EQ(1, project->getSchematics().count());
    EXPECT_EQ(1, project->getBoards().count());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace eagleimport
}  // namespace librepcb
