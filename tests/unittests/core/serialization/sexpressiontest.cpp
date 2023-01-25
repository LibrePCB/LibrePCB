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
#include <librepcb/core/serialization/sexpression.h>

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
  EXPECT_EQ("foo bar", s.getChild("@0").getValue());
}

TEST(SExpressionTest, testParseStringWithQuotes) {
  SExpression s = SExpression::parse("(test \"foo \\\"bar\\\"\")", FilePath());
  EXPECT_TRUE(s.isList());
  EXPECT_EQ(1, s.getChildren().count());
  EXPECT_EQ("foo \"bar\"", s.getChild("@0").getValue());
}

TEST(SExpressionTest, testParseStringWithNewlines) {
  SExpression s = SExpression::parse("(test \"foo\\nbar\")", FilePath());
  EXPECT_TRUE(s.isList());
  EXPECT_EQ(1, s.getChildren().count());
  EXPECT_EQ("foo\nbar", s.getChild("@0").getValue());
}

TEST(SExpressionTest, testParseStringWithBackslash) {
  SExpression s = SExpression::parse("(test \"foo\\\\bar\")", FilePath());
  EXPECT_TRUE(s.isList());
  EXPECT_EQ(1, s.getChildren().count());
  EXPECT_EQ("foo\\bar", s.getChild("@0").getValue());
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
  EXPECT_EQ("newstroke.bene", s.getChild("default_font/@0").getValue());
  EXPECT_EQ("0.15875", s.getChild("grid/interval/@0").getValue());
  EXPECT_EQ("./output/{{VERSION}}/gerber/{{PROJECT}}",
            s.getChild("fabrication_output_settings/base_path/@0").getValue());
  EXPECT_EQ(
      "",
      s.getChild("fabrication_output_settings/outlines/suffix/@0").getValue());
  EXPECT_EQ(".gto",
            s.getChild("fabrication_output_settings/silkscreen_top/suffix/@0")
                .getValue());
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

TEST(SExpressionTest, testRoundtrip) {
  // Create input with wrong indentation, this shall be fixed by toByteArray().
  QByteArray input =
      "(librepcb_board 71762d7e-e7f1-403c-8020-db9670c01e9b\n"
      "(default_font \"newstroke.bene\")\n"
      "(grid (type lines) (interval 0.15875) (unit millimeters))\n"
      "(fabrication_output_settings\n"
      "(base_path \"./output/{{VERSION}}/gerber/{{PROJECT}}\")\n"
      "(outlines (suffix \"\"))\n"
      "  (silkscreen_top (suffix \".gto\")\n"
      "    (layers top_placement top_names)\n"
      "  )\n"
      ")\n"
      " (emptylines foo\n"
      " \n "
      "     (child 1)\n"
      " \n "
      "\n"
      "  )\n"
      "(multiline foo\n"
      ")\n"
      "(emptyline\n"
      ")\n"
      "(empty)\n"
      ")\n";
  SExpression s = SExpression::parse(input, FilePath());
  QByteArray actual = s.toByteArray();
  QByteArray expected =
      "(librepcb_board 71762d7e-e7f1-403c-8020-db9670c01e9b\n"
      " (default_font \"newstroke.bene\")\n"
      " (grid (type lines) (interval 0.15875) (unit millimeters))\n"
      " (fabrication_output_settings\n"
      "  (base_path \"./output/{{VERSION}}/gerber/{{PROJECT}}\")\n"
      "  (outlines (suffix \"\"))\n"
      "  (silkscreen_top (suffix \".gto\")\n"
      "   (layers top_placement top_names)\n"
      "  )\n"
      " )\n"
      " (emptylines foo\n"
      "\n"
      "  (child 1)\n"
      "\n"
      "\n"
      " )\n"
      " (multiline foo\n"
      " )\n"
      " (emptyline\n"
      " )\n"
      " (empty)\n"
      ")\n";
  EXPECT_EQ(expected.toStdString(), actual.toStdString());
}

TEST(SExpressionTest, testGetChildSkipsLineBreaks) {
  SExpression s =
      SExpression::parse("(root \n (child \n 0 \n 1 \n 2 \n ))", FilePath());
  EXPECT_EQ("0", s.getChild("child/@0").getValue().toStdString());
  EXPECT_EQ("1", s.getChild("child/@1").getValue().toStdString());
  EXPECT_EQ("2", s.getChild("child/@2").getValue().toStdString());
}

TEST(SExpressionTest, testRemoveChild) {
  const QByteArray input =
      "(test value\n"
      " (child1 a b c)\n"
      " (child2 a b c)\n"
      ")\n";
  SExpression s = SExpression::parse(input, FilePath());
  SExpression& child = s.getChild("child1");
  s.removeChild(child);
  const QByteArray actual = s.toByteArray();
  const QByteArray expected =
      "(test value\n\n"
      " (child2 a b c)\n"
      ")\n";
  EXPECT_EQ(expected.toStdString(), actual.toStdString());
}

TEST(SExpressionTest, testRemoveInvalidChild) {
  const QByteArray input =
      "(test value\n"
      " (child1 a b c)\n"
      " (child2 a b c)\n"
      ")\n";
  SExpression s = SExpression::parse(input, FilePath());
  SExpression& child = s.getChild("child1/@0");
  EXPECT_THROW(s.removeChild(child), LogicError);
}

TEST(SExpressionTest, testToByteArrayEmptyList) {
  SExpression s = SExpression::createList("test");
  EXPECT_EQ("(test)\n", s.toByteArray().toStdString());
}

TEST(SExpressionTest, testToByteArrayEmptyListWithTrailingLineBreak) {
  SExpression s = SExpression::createList("test");
  s.ensureLineBreak();
  EXPECT_EQ(
      "(test\n"
      ")\n",
      s.toByteArray().toStdString());
}

TEST(SExpressionTest, testToByteArrayListWithLineBreaks) {
  SExpression s = SExpression::createList("test");
  s.appendChild("child", SExpression::createToken("1"));
  s.ensureLineBreak();
  s.appendChild("child", SExpression::createToken("2"));
  s.ensureLineBreak();
  EXPECT_EQ(
      "(test (child 1)\n"
      " (child 2)\n"
      ")\n",
      s.toByteArray().toStdString());
}

TEST(SExpressionTest, testToByteArrayListWithTooManyLineBreaks) {
  SExpression s = SExpression::createList("test");
  s.appendChild("child", SExpression::createToken("1"));
  s.ensureLineBreak();
  s.ensureLineBreak();
  s.ensureLineBreak();
  s.appendChild("child", SExpression::createToken("2"));
  s.ensureLineBreak();
  s.ensureLineBreak();
  s.ensureLineBreak();
  EXPECT_EQ(
      "(test (child 1)\n"
      " (child 2)\n"
      ")\n",
      s.toByteArray().toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
