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

#include "../../testhelpers.h"

#include <gtest/gtest.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/editor/editorcommandset.h>
#include <librepcb/editor/utils/shortcutsreferencegenerator.h>

#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

using ::librepcb::tests::TestHelpers;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ShortcutsReferenceGeneratorTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ShortcutsReferenceGeneratorTest, testExportPdfMultipleTimes) {
  FilePath fp = FilePath::getRandomTempPath().getPathTo("test.pdf");
  ShortcutsReferenceGenerator gen(EditorCommandSet::instance());
  gen.generatePdf(fp);
  EXPECT_TRUE(fp.isExistingFile());

  // Export again to see if it doesn't fail on already existing file.
  gen.generatePdf(fp);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
