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
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/editor/library/cmd/cmdsymbolreload.h>

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

class CmdSymbolReloadTest : public ::testing::Test {
protected:
  FilePath mTmpDir;
  CmdSymbolReloadTest() : mTmpDir(FilePath::getRandomTempPath()) {}
  ~CmdSymbolReloadTest() { QDir(mTmpDir.toStr()).removeRecursively(); }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(CmdSymbolReloadTest, test) {
  const QByteArray content =
      R"((librepcb_symbol acd99b30-59a5-419f-b067-ae704e4364bb
 (name "New Name")
 (description "New Description")
 (keywords "New Keywords")
 (author "New Author")
 (version "0.2")
 (created 2015-06-21T12:37:34Z)
 (deprecated true)
 (generated_by "New Generated")
 (category 414f873f-4099-47fd-8526-bdd8419de581)
 (pin 6a5d679d-2f42-4af4-b9e3-e4ae3fd20080 (name "NewPin")
  (position 0.0 2.54) (rotation 270.0) (length 1.524)
  (name_position 2.794 0.0) (name_rotation 0.0) (name_height 2.5)
  (name_align left center)
 )
 (polygon 236abe33-aa52-479b-b2b9-f4ac81bb49f8 (layer top_documentation)
  (width 0.1016) (fill false) (grab_area true)
  (vertex (position -0.381 0.66) (angle 0.0))
  (vertex (position 0.381 0.66) (angle 0.0))
 )
 (circle d097e468-94ec-4266-ae81-df16b4b177cb (layer top_documentation)
  (width 0.5) (fill false) (grab_area true) (diameter 3.0) (position 1.1 2.2)
 )
 (text 251278b2-6533-4783-907e-55c51594ae5c (layer sym_values) (value "New Value")
  (align left top) (height 2.54) (position 2.54 0.0) (rotation 0.0)
 )
)
)";

  // Create file system for the library element.
  std::shared_ptr<TransactionalFileSystem> fs = TransactionalFileSystem::openRW(
      mTmpDir.getPathTo("acd99b30-59a5-419f-b067-ae704e4364bb"));
  std::unique_ptr<TransactionalDirectory> dir(new TransactionalDirectory(fs));

  // Create a "empty" library element and save it to the file system.
  std::unique_ptr<Symbol> element(
      new Symbol(Uuid::fromString("acd99b30-59a5-419f-b067-ae704e4364bb"),
                 Version::fromString("0.1"), "", ElementName("name"), "", ""));
  element->saveTo(*dir);
  fs->save();

  // Check that the file has been written.
  const FilePath lpFile =
      mTmpDir.getPathTo("acd99b30-59a5-419f-b067-ae704e4364bb/symbol.lp");
  EXPECT_TRUE(lpFile.isExistingFile());

  // Now overwrite the file with a library element that uses all features.
  FileUtils::writeFile(lpFile, content);

  // Reload the library element.
  CmdSymbolReload cmd(*element);
  const bool ret = cmd.execute();
  EXPECT_TRUE(ret);

  // Save the library element again and verify the content matches. This will
  // fail if any library element property has not been reloaded properly.
  FileUtils::removeFile(lpFile);
  element->save();
  fs->save();
  const QByteArray newContent = FileUtils::readFile(lpFile);
  EXPECT_EQ(newContent.toStdString(), content.toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
