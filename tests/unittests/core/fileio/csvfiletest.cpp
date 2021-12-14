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
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/csvfile.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class CsvFileTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(CsvFileTest, testDefaultConstructor) {
  CsvFile f;
  EXPECT_EQ("", f.getComment().toStdString());
  EXPECT_EQ(0, f.getHeader().count());
  EXPECT_EQ(0, f.getValues().count());
  EXPECT_EQ("", f.toString().toStdString());
}

TEST_F(CsvFileTest, testCommentOnly) {
  CsvFile f;
  f.setComment("Foo\n\nBar");
  EXPECT_EQ(
      "# Foo\n"
      "#\n"
      "# Bar\n"
      "\n",
      f.toString().toStdString());
}

TEST_F(CsvFileTest, testHeaderOnly) {
  CsvFile f;
  f.setHeader({"Foo", "Bar"});
  EXPECT_EQ("Foo,Bar\n", f.toString().toStdString());
}

TEST_F(CsvFileTest, testSetHeaderClearsValues) {
  CsvFile f;
  f.setHeader({"Foo", "Bar"});
  f.addValue({"V1", "V2"});
  EXPECT_EQ(1, f.getValues().count());
  f.setHeader({"Foo", "Bar"});
  EXPECT_EQ(0, f.getValues().count());
}

TEST_F(CsvFileTest, testAddValuesThrowsExceptionIfNoHeaderSet) {
  CsvFile f;
  EXPECT_THROW(f.addValue({"V1", "V2"}), Exception);
}

TEST_F(CsvFileTest, testAddValuesThrowsExceptionIfWrongCount) {
  CsvFile f;
  f.setHeader({"Foo"});
  EXPECT_THROW(f.addValue({"V1", "V2"}), Exception);
}

TEST_F(CsvFileTest, testToStringWithQuotingAndEscaping) {
  CsvFile f;
  f.setComment("Foo\nBar");
  f.setHeader({"Column", "Column With Space", "With,Comma", "\"With Quotes\""});
  f.addValue({"", "", "", ""});
  f.addValue({"Value", "Value With Space", "With,Comma", "\"With Quotes\""});
  f.addValue({"-1.2345", "Foo\r\nBar", " spaces around ", "äöü"});
  EXPECT_EQ(
      "# Foo\n"
      "# Bar\n"
      "\n"
      "Column,Column With Space,\"With,Comma\",\"\"\"With Quotes\"\"\"\n"
      ",,,\n"
      "Value,Value With Space,\"With,Comma\",\"\"\"With Quotes\"\"\"\n"
      "-1.2345,Foo Bar, spaces around ,äöü\n",
      f.toString().toStdString());
}

TEST_F(CsvFileTest, testSaveToFile) {
  CsvFile f;
  f.setComment("Foo\nBar");
  f.setHeader({"Column", "Column With Space", "With,Comma", "\"With Quotes\""});
  f.addValue({"", "", "", ""});
  f.addValue({"Value", "Value With Space", "With,Comma", "\"With Quotes\""});
  f.addValue({"-1.2345", "Foo\r\nBar", " spaces around ", "äöü"});

  FilePath fp = FilePath::getRandomTempPath();
  f.saveToFile(fp);
  EXPECT_EQ(
      "# Foo\n"
      "# Bar\n"
      "\n"
      "Column,Column With Space,\"With,Comma\",\"\"\"With Quotes\"\"\"\n"
      ",,,\n"
      "Value,Value With Space,\"With,Comma\",\"\"\"With Quotes\"\"\"\n"
      "-1.2345,Foo Bar, spaces around ,äöü\n",
      FileUtils::readFile(fp));
  QFile(fp.toStr()).remove();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
