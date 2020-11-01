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
#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SExpressionTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(SExpressionTest, testParseEmptyBytearray) {
  EXPECT_THROW(SExpression::parse("", FilePath()), RuntimeError);
}

TEST(SExpressionTest, testParseEmptyBraces) {
  EXPECT_THROW(SExpression::parse("()", FilePath()), RuntimeError);
}

TEST(SExpressionTest, testParseMissingClosingBrace) {
  EXPECT_THROW(SExpression::parse("(test", FilePath()), RuntimeError);
}

TEST(SExpressionTest, testParseTooFewClosingBraces) {
  EXPECT_THROW(SExpression::parse("(test (foo bar)", FilePath()), RuntimeError);
}

TEST(SExpressionTest, testParseTooManyClosingBraces) {
  EXPECT_THROW(SExpression::parse("(test (foo bar)))", FilePath()),
               RuntimeError);
}

TEST(SExpressionTest, testParseEmptyList) {
  SExpression s = SExpression::parse("(test)", FilePath());
  EXPECT_TRUE(s.isList());
}

TEST(SExpressionTest, testParseStringWithMissingEndQuote) {
  EXPECT_THROW(SExpression::parse("(test \"foo)", FilePath()), RuntimeError);
}

TEST(SExpressionTest, testParseString) {
  SExpression s = SExpression::parse("(test \"foo bar\")", FilePath());
  EXPECT_TRUE(s.isList());
  EXPECT_EQ(1, s.getChildren().count());
  EXPECT_EQ("foo bar", s.getChildByIndex(0).getStringOrToken());
}

TEST(SExpressionTest, testParseStringWithQuotes) {
  SExpression s = SExpression::parse("(test \"foo \\\"bar\\\"\")", FilePath());
  EXPECT_TRUE(s.isList());
  EXPECT_EQ(1, s.getChildren().count());
  EXPECT_EQ("foo \"bar\"", s.getChildByIndex(0).getStringOrToken());
}

TEST(SExpressionTest, testParseStringWithNewlines) {
  SExpression s = SExpression::parse("(test \"foo\\nbar\")", FilePath());
  EXPECT_TRUE(s.isList());
  EXPECT_EQ(1, s.getChildren().count());
  EXPECT_EQ("foo\nbar", s.getChildByIndex(0).getStringOrToken());
}

TEST(SExpressionTest, testParseStringWithBackslash) {
  SExpression s = SExpression::parse("(test \"foo\\\\bar\")", FilePath());
  EXPECT_TRUE(s.isList());
  EXPECT_EQ(1, s.getChildren().count());
  EXPECT_EQ("foo\\bar", s.getChildByIndex(0).getStringOrToken());
}

TEST(SExpressionTest, testParseExpressionWithChildrenAndComments) {
  QByteArray input =
      "; (This whole line is a comment with CRLF line ending)\r\n"
      "(librepcb_board 71762d7e-e7f1-403c-8020-db9670c01e9b\n"
      " (default_font \"newstroke.bene\")\n"
      " (grid (type lines) (interval 0.15875) (unit millimeters))\n"
      " (fabrication_output_settings ; \"Just a comment\"\n"
      "  (base_path \"./output/{{VERSION}}/gerber/{{PROJECT}}\")\n"
      "  (outlines (suffix \"\"))\n"
      "  (silkscreen_top (suffix \".gto\")\n"
      "   (layers top_placement top_names)\n"
      "  )\n"
      " )\n"
      ")\n";
  SExpression s = SExpression::parse(input, FilePath());
  EXPECT_EQ("newstroke.bene", s.getValueByPath<QString>("default_font"));
  EXPECT_EQ("0.15875", s.getValueByPath<QString>("grid/interval"));
  EXPECT_EQ("./output/{{VERSION}}/gerber/{{PROJECT}}",
            s.getValueByPath<QString>("fabrication_output_settings/base_path"));
  EXPECT_EQ(
      "",
      s.getValueByPath<QString>("fabrication_output_settings/outlines/suffix"));
  EXPECT_EQ(".gto",
            s.getValueByPath<QString>(
                "fabrication_output_settings/silkscreen_top/suffix"));
}

TEST(SExpressionTest, testParsePartialExpression) {
  QByteArray input =
      "(librepcb_board 71762d7e-e7f1-403c-8020-db9670c01e9b\n"
      " (default_font \"newstroke.bene\")\n"
      " (grid (type lines) (interval 0.15875) (unit millimeters))\n"
      " (fabrication_output_settings ; \"Just a comment\"\n"
      "  (base_path \"./output/{{VERSION}}/gerber/{{PROJECT}}\")\n"
      "  (outlines (suffix \"\"))\n"
      "  (silkscreen_top (suffix \".gto\")\n"
      "   (layers top_placement top_names)\n"
      "  )\n"
      " )\n"
      ")";  // final newline omitted

  // Check if parsing fails at *any* character boundary of the input string.
  // This test is mainly there to check if the application does not crash due
  // to index out of bounds string access.
  for (int i = 0; i < input.length(); ++i) {
    EXPECT_THROW(SExpression::parse(input.left(i), FilePath()), RuntimeError);
    EXPECT_THROW(SExpression::parse(input.right(i), FilePath()), RuntimeError);
  }
}

TEST(SExpressionTest, testSerializeStringWithEscaping) {
  SExpression s = SExpression::createString("Foo\n \r\n \" \\ Bar");
  EXPECT_EQ("\"Foo\\n \\r\\n \\\" \\\\ Bar\"\n", s.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
