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
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/editor/utils/slinthelpers.h>
#include <librepcb/editor/workspace/filesystemmodel.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class FileSystemModelTest : public ::testing::Test {
public:
  FilePath mWsDir;
  std::unique_ptr<Workspace> mWs;

  FileSystemModelTest() : mWsDir(Application::getRandomTempPath()) {
    Workspace::createNewWorkspace(mWsDir);
    mWs = std::make_unique<Workspace>(mWsDir, "data");
  }

  ~FileSystemModelTest() override {
    QDir(mWsDir.getParentDir().toStr()).removeRecursively();
  }

  void createEntries(const QStringList& entries) {
    for (const QString& entry : entries) {
      if (entry.endsWith("/")) {
        FileUtils::makePath(mWs->getProjectsPath().getPathTo(entry));
      } else {
        FileUtils::writeFile(mWs->getProjectsPath().getPathTo(entry), "");
      }
    }
  }

  std::string str(const QStringList& texts) {
    return texts.join(", ").toStdString();
  }
};

/*******************************************************************************
 *  Tests
 ******************************************************************************/

// Regression test for https://github.com/LibrePCB/LibrePCB/issues/1923:
// Numbers must be compared numerically and the '_' character must not break
// the natural sort order.
TEST_F(FileSystemModelTest, testSortOrder) {
  createEntries({
      "10/",
      "2/",
      "1/",
      "3.txt",
      "10.txt",
      "_10.txt",
      "Project_10/",
      "project_2/",
      "_Project_1/",
      "Project_1/",
  });

  FileSystemModel model(*mWs, mWs->getProjectsPath(), "test", nullptr);
  QStringList items;
  for (std::size_t i = 0; i < model.row_count(); ++i) {
    if (auto item = model.row_data(i)) {
      items.append(s2q(item->text));
    }
  }
  EXPECT_EQ(str({
                "_Project_1",
                "1",
                "2",
                "10",
                "Project_1",
                "project_2",
                "Project_10",
                "_10.txt",
                "3.txt",
                "10.txt",
            }),
            str(items));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
