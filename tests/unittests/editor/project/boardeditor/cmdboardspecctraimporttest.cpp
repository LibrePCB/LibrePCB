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
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/editor/project/cmd/cmdboardspecctraimport.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class CmdBoardSpecctraImportTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(CmdBoardSpecctraImportTest, test) {
  FilePath testDataDir(TEST_DATA_DIR
                       "/unittests/librepcbproject/BoardSpecctraExportTest");

  // Open project from test data directory.
  FilePath projectFp(TEST_DATA_DIR "/projects/Gerber Test/project.lpp");
  std::shared_ptr<TransactionalFileSystem> projectFs =
      TransactionalFileSystem::openRO(projectFp.getParentDir());
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::unique_ptr<TransactionalDirectory>(
                      new TransactionalDirectory(projectFs)),
                  projectFp.getFilename());  // can throw

  // Load SES.
  const FilePath fp = testDataDir.getPathTo("session.ses");
  std::unique_ptr<SExpression> root = SExpression::parse(
      FileUtils::readFile(fp), fp, SExpression::Mode::Permissive);

  // Import SES.
  Board* board = project->getBoards().first();
  std::shared_ptr<MessageLogger> log = std::make_shared<MessageLogger>();
  CmdBoardSpecctraImport imp(*board, *root, log);
  const bool ret = imp.execute();
  EXPECT_TRUE(ret);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
