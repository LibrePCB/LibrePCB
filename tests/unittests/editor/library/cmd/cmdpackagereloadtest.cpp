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
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/editor/library/cmd/cmdpackagereload.h>

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

class CmdPackageReloadTest : public ::testing::Test {
protected:
  FilePath mTmpDir;
  CmdPackageReloadTest() : mTmpDir(FilePath::getRandomTempPath()) {}
  ~CmdPackageReloadTest() { QDir(mTmpDir.toStr()).removeRecursively(); }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(CmdPackageReloadTest, test) {
  const QByteArray content =
      R"((librepcb_package acd99b30-59a5-419f-b067-ae704e4364bb
 (name "New Name")
 (description "New Description")
 (keywords "New Keywords")
 (author "New Author")
 (version "0.2")
 (created 2015-06-21T12:37:34Z)
 (deprecated true)
 (generated_by "New Generated")
 (category 414f873f-4099-47fd-8526-bdd8419de581)
 (alternative_name "New" (reference "Alternative"))
 (assembly_type tht)
 (pad 175b71f7-b284-4c31-b05f-b0aa64ad48e0 (name "NewPad"))
 (footprint 2b7ac931-7855-4f1e-bbfe-c07f2c6c0d89
  (name "New Fpt Name")
  (description "New Fpt Desc")
  (3d_position 1.0 2.0 3.0) (3d_rotation 4.0 5.0 6.0)
  (pad 175b71f7-b284-4c31-b05f-b0aa64ad48e0 (side top) (shape roundrect)
   (position -0.85 0.0) (rotation 0.0) (size 1.3 1.5) (radius 0.0)
   (stop_mask auto) (solder_paste auto) (clearance 0.0) (function unspecified)
   (package_pad 175b71f7-b284-4c31-b05f-b0aa64ad48e0)
  )
  (polygon 236abe33-aa52-479b-b2b9-f4ac81bb49f8 (layer top_documentation)
   (width 0.1016) (fill false) (grab_area true)
   (vertex (position -0.381 0.66) (angle 0.0))
   (vertex (position 0.381 0.66) (angle 0.0))
  )
  (circle d097e468-94ec-4266-ae81-df16b4b177cb (layer top_documentation)
   (width 0.5) (fill false) (grab_area true) (diameter 3.0) (position 1.1 2.2)
  )
  (stroke_text 0dc2263b-d972-47f1-bdf1-742e632c24f5 (layer top_names)
   (height 1.0) (stroke_width 0.2) (letter_spacing auto) (line_spacing auto)
   (align left bottom) (position -1.27 1.27) (rotation 0.0)
   (auto_rotate true) (mirror false) (value "{{NAME}}")
  )
 )
)
)";

  // Create file system for the library element.
  std::shared_ptr<TransactionalFileSystem> fs = TransactionalFileSystem::openRW(
      mTmpDir.getPathTo("acd99b30-59a5-419f-b067-ae704e4364bb"));
  std::unique_ptr<TransactionalDirectory> dir(new TransactionalDirectory(fs));

  // Create a "empty" library element and save it to the file system.
  std::unique_ptr<Package> element(
      new Package(Uuid::fromString("acd99b30-59a5-419f-b067-ae704e4364bb"),
                  Version::fromString("0.1"), "", ElementName("name"), "", "",
                  Package::AssemblyType::Auto));
  element->saveTo(*dir);
  fs->save();

  // Check that the file has been written.
  const FilePath lpFile =
      mTmpDir.getPathTo("acd99b30-59a5-419f-b067-ae704e4364bb/package.lp");
  EXPECT_TRUE(lpFile.isExistingFile());

  // Now overwrite the file with a library element that uses all features.
  FileUtils::writeFile(lpFile, content);

  // Reload the library element.
  CmdPackageReload cmd(*element);
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
