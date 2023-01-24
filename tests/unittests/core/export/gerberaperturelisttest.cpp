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
#include <librepcb/core/export/gerberaperturelist.h>

#include <QRegularExpression>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GerberApertureListTest : public ::testing::Test {
protected:
  /**
   * @brief Helper function to generate all possible apertures
   *
   * @return Apertures
   */
  static const QString& generateEverything() noexcept {
    static QString s;
    if (s.isEmpty()) {
      GerberApertureList l;

      QVector<GerberApertureList::Function> functions = {
          tl::nullopt,
          tl::make_optional(GerberAttribute::ApertureFunction::Conductor),
      };

      for (auto function : functions) {
        l.addCircle(UnsignedLength(0), function);
        l.addCircle(UnsignedLength(100000), function);

        for (int i = -359; i <= 359; ++i) {
          Angle rot = Angle(i * 1000000);  // -359..+359°

          l.addObround(PositiveLength(100000), PositiveLength(100000), rot,
                       function);
          l.addObround(PositiveLength(100000), PositiveLength(200000), rot,
                       function);
          l.addObround(PositiveLength(200000), PositiveLength(100000), rot,
                       function);

          // No rounded corners.
          l.addRect(PositiveLength(100000), PositiveLength(100000),
                    UnsignedLength(0), rot, function);
          l.addRect(PositiveLength(100000), PositiveLength(200000),
                    UnsignedLength(0), rot, function);
          l.addRect(PositiveLength(200000), PositiveLength(100000),
                    UnsignedLength(0), rot, function);

          // Rounded corners.
          l.addRect(PositiveLength(100000), PositiveLength(100000),
                    UnsignedLength(20000), rot, function);
          l.addRect(PositiveLength(100000), PositiveLength(200000),
                    UnsignedLength(20000), rot, function);
          l.addRect(PositiveLength(200000), PositiveLength(100000),
                    UnsignedLength(20000), rot, function);

          // No rounded corners.
          l.addOctagon(PositiveLength(100000), PositiveLength(100000),
                       UnsignedLength(0), rot, function);
          l.addOctagon(PositiveLength(100000), PositiveLength(200000),
                       UnsignedLength(0), rot, function);
          l.addOctagon(PositiveLength(200000), PositiveLength(100000),
                       UnsignedLength(0), rot, function);

          // Rounded corners.
          l.addOctagon(PositiveLength(100000), PositiveLength(100000),
                       UnsignedLength(20000), rot, function);
          l.addOctagon(PositiveLength(100000), PositiveLength(200000),
                       UnsignedLength(20000), rot, function);
          l.addOctagon(PositiveLength(200000), PositiveLength(100000),
                       UnsignedLength(20000), rot, function);
        }
      }
      s = l.generateString();
    }
    return s;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

// Check some general syntax rules of the %ADD command.
TEST_F(GerberApertureListTest, testApertureDefinitionSyntax) {
  int checkedLines = 0;
  QString s = generateEverything();
  foreach (const QString line, s.split('\n')) {
    if (line.contains("%ADD")) {
      QRegularExpression re("^%ADD(\\d+).+\\*%$");
      QRegularExpressionMatch match = re.match(line);
      EXPECT_TRUE(match.hasMatch());
      EXPECT_GE(match.captured(1).toInt(), 10);
      ++checkedLines;
    }
  }
  ASSERT_GE(checkedLines, 3);  // Sanity check if test works.
}

// Check some general syntax rules of the %AM command.
TEST_F(GerberApertureListTest, testApertureMacroSyntax) {
  int checkedLines = 0;
  QString s = generateEverything();
  foreach (const QString line, s.split('\n')) {
    if (line.contains("%AM")) {
      QRegularExpression re("^%AM.+\\*%$");
      QRegularExpressionMatch match = re.match(line);
      EXPECT_TRUE(match.hasMatch());
      ++checkedLines;
    }
  }
  ASSERT_GE(checkedLines, 3);  // Sanity check if test works.
}

// Check if we never use regular polygon macros. Some tools wrongly assume that
// the specified diameter is the *inner* diameter, but it's the *outer*
// diameter (as stated in the Gerber specs). So we should really not rely on
// an aperture type which is sometimes misinterpreted.
TEST_F(GerberApertureListTest, testNotUsingRegularPolygon) {
  QString s = generateEverything();

  // Sanity check if the regex works as expected (there will be circles).
  ASSERT_GE(s.count(QRegularExpression("%ADD\\d+C")), 2);

  // Now check for absence of polygons (same as above, just "P" instead of "C").
  EXPECT_EQ(s.count(QRegularExpression("%ADD\\d+P")), 0);
}

// Check if we never use aperture macro variables. Such variables might cause
// issues in some CAM software (not 100% sure, but to be on the safe side, we
// still avoid them).
TEST_F(GerberApertureListTest, testNotUsingMacroVariables) {
  QString s = generateEverything();
  EXPECT_FALSE(s.contains("$1"));
}

// Check if we never use arithmetic expressions in aperture macros. Such
// expressions cause lots of issues in some CAM software, so DON'T USE THEM!!!
TEST_F(GerberApertureListTest, testNotUsingArithmeticExpressions) {
  QString s = generateEverything();

  // Find all circles inside aperture macros.
  int foundMacros = 0;
  QRegularExpression re("%AM.*?%",
                        QRegularExpression::DotMatchesEverythingOption);
  auto i = re.globalMatch(s);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString macro = match.captured();
    EXPECT_FALSE(macro.contains("("));
    EXPECT_FALSE(macro.contains(")"));
    EXPECT_FALSE(macro.contains("+"));
    EXPECT_FALSE(macro.contains("x"));
    EXPECT_FALSE(macro.contains("X"));
    EXPECT_FALSE(macro.contains("/"));
    ++foundMacros;
  }

  // Sanity check if the test works properly. There *must* be some macros
  // found!
  ASSERT_GE(foundMacros, 3);
}

// Check if circles in aperture macros do not specify the rotation parameter.
// This parameter causes issues in some CAM software, so DON'T USE THEM!!!
TEST_F(GerberApertureListTest, testNotUsingMacroCircleRotation) {
  QString s = generateEverything();

  // Find all circles inside aperture macros.
  int foundMacros = 0;
  int foundCircles = 0;
  QRegularExpression re("%AM.*?%",
                        QRegularExpression::DotMatchesEverythingOption);
  auto i = re.globalMatch(s);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QStringList macroItems = match.captured().split('*');
    foreach (const QString& item, macroItems) {
      if (item.startsWith("1,")) {
        ++foundCircles;
        EXPECT_EQ(item.count(','), 4);  // No rotation parameter!!!
      }
    }
    ++foundMacros;
  }

  // Sanity check if the test works properly. There *must* be some circles
  // found!
  ASSERT_GE(foundMacros, 3);
  ASSERT_GE(foundCircles, 3);
}

// Check if we never use the macro primitive "Center Line, Code 21" since some
// tools implement its rotation wrong (see explanation in Gerber specs). It
// might be safe to use center lines without rotation, but let's avoid this
// primitive entirely since there are good alternatives available.
TEST_F(GerberApertureListTest, testNotUsingMacroCenterLine) {
  QString s = generateEverything();

  // Find all center lines inside aperture macros.
  int foundMacros = 0;
  int foundCenterLines = 0;
  QRegularExpression re("%AM.*?%",
                        QRegularExpression::DotMatchesEverythingOption);
  auto i = re.globalMatch(s);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QStringList macroItems = match.captured().split('*');
    foreach (const QString& item, macroItems) {
      if (item.startsWith("21,")) {
        ++foundCenterLines;
      }
    }
    ++foundMacros;
  }

  // Sanity check if the test works properly.
  ASSERT_GE(foundMacros, 3);

  // Check if no center lines were used.
  ASSERT_EQ(foundCenterLines, 0);
}

// Test if the same aperture ID is returned when creating multiple apertures
// with the same properties and attributes.
TEST_F(GerberApertureListTest, testSamePropertiesAndAttributes) {
  GerberApertureList l;

  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());

  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());
}

// Test if a new aperture ID is returned when creating multiple apertures
// with different properties but with the same attributes.
TEST_F(GerberApertureListTest, testDifferentProperties) {
  GerberApertureList l;

  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());

  EXPECT_EQ(11, l.addCircle(UnsignedLength(100000), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n%ADD11C,0.1*%\n", l.generateString().toStdString());
}

// Test if a new aperture ID is returned when creating multiple apertures
// with the same properties but with different attributes.
TEST_F(GerberApertureListTest, testDifferentAttributes) {
  GerberApertureList l;

  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());

  const char* expected =
      "%ADD10C,0.0*%\n"
      "G04 #@! TA.AperFunction,Conductor*\n"
      "%ADD11C,0.0*%\n"
      "G04 #@! TD*\n";
  EXPECT_EQ(11,
            l.addCircle(UnsignedLength(0),
                        GerberAttribute::ApertureFunction::Conductor));
  EXPECT_EQ(expected, l.generateString().toStdString());
}

// Test if a new aperture ID is returned when creating multiple apertures
// with different properties and with different attributes.
TEST_F(GerberApertureListTest, testDifferentPropertiesAndAttributes) {
  GerberApertureList l;

  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());

  const char* expected =
      "%ADD10C,0.0*%\n"
      "G04 #@! TA.AperFunction,Conductor*\n"
      "%ADD11C,0.1*%\n"
      "G04 #@! TD*\n";
  EXPECT_EQ(11,
            l.addCircle(UnsignedLength(100000),
                        GerberAttribute::ApertureFunction::Conductor));
  EXPECT_EQ(expected, l.generateString().toStdString());
}

// Test if the attributes get deleted at the end of the aperture list, but only
// if it was set before.
TEST_F(GerberApertureListTest, testAttributesGetDeletedAtEnd) {
  GerberApertureList l;

  // No attribute set -> nothing to clear.
  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());

  // Attribute set -> must be cleared at end.
  const char* expected =
      "%ADD10C,0.0*%\n"
      "G04 #@! TA.AperFunction,Conductor*\n"
      "%ADD11C,0.1*%\n"
      "G04 #@! TD*\n";
  EXPECT_EQ(11,
            l.addCircle(UnsignedLength(100000),
                        GerberAttribute::ApertureFunction::Conductor));
  EXPECT_EQ(expected, l.generateString().toStdString());

  // Last aperture has no attribute -> nothing to clear.
  expected =
      "%ADD10C,0.0*%\n"
      "G04 #@! TA.AperFunction,Conductor*\n"
      "%ADD11C,0.1*%\n"
      "G04 #@! TD*\n"
      "%ADD12C,0.2*%\n";
  EXPECT_EQ(12, l.addCircle(UnsignedLength(200000), tl::nullopt));
  EXPECT_EQ(expected, l.generateString().toStdString());
}

// Test if a circle of size 0 (which is allowed) is exported according specs.
TEST_F(GerberApertureListTest, testCircleDiameterZero) {
  GerberApertureList l;

  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());

  // Set same aperture again to see if it gets reused.
  EXPECT_EQ(10, l.addCircle(UnsignedLength(0), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n", l.generateString().toStdString());

  // Set another size to see if a new aperture gets created.
  EXPECT_EQ(11, l.addCircle(UnsignedLength(100000), tl::nullopt));
  EXPECT_EQ("%ADD10C,0.0*%\n%ADD11C,0.1*%\n", l.generateString().toStdString());
}

// Test if a circle of size >0 is exported according specs.
TEST_F(GerberApertureListTest, testCircleDiameterNonZero) {
  GerberApertureList l;

  EXPECT_EQ(10, l.addCircle(UnsignedLength(1230000), tl::nullopt));
  EXPECT_EQ("%ADD10C,1.23*%\n", l.generateString().toStdString());

  // Set same aperture again to see if it gets reused.
  EXPECT_EQ(10, l.addCircle(UnsignedLength(1230000), tl::nullopt));
  EXPECT_EQ("%ADD10C,1.23*%\n", l.generateString().toStdString());

  // Set another size to see if a new aperture gets created.
  EXPECT_EQ(11, l.addCircle(UnsignedLength(100000), tl::nullopt));
  EXPECT_EQ("%ADD10C,1.23*%\n%ADD11C,0.1*%\n",
            l.generateString().toStdString());
}

// Test if an obround with width==height is exported as a circle since the
// circle is simpler and thus more robust and more efficient.
TEST_F(GerberApertureListTest, testObroundSameSize) {
  GerberApertureList l;

  EXPECT_EQ(10,
            l.addObround(PositiveLength(1230000), PositiveLength(1230000),
                         Angle::deg0(), tl::nullopt));
  EXPECT_EQ("%ADD10C,1.23*%\n", l.generateString().toStdString());

  // Set same aperture again to see if it gets reused.
  EXPECT_EQ(10,
            l.addObround(PositiveLength(1230000), PositiveLength(1230000),
                         Angle::deg90(), tl::nullopt));
  EXPECT_EQ("%ADD10C,1.23*%\n", l.generateString().toStdString());

  // Set another size to see if a new aperture gets created.
  EXPECT_EQ(11,
            l.addObround(PositiveLength(100000), PositiveLength(100000),
                         Angle::deg90(), tl::nullopt));
  EXPECT_EQ("%ADD10C,1.23*%\n%ADD11C,0.1*%\n",
            l.generateString().toStdString());
}

// Test if an obround with height>width and rotation=0°;+/-180° is exported as a
// simple obround aperture.
TEST_F(GerberApertureListTest, testHighObround0deg) {
  GerberApertureList l;
  PositiveLength w(100000);
  PositiveLength h(200000);
  QVector<Angle> rotations = {
      -Angle::deg180(),
      Angle::deg0(),
      Angle::deg180(),
  };

  const char* expected = "%ADD10O,0.1X0.2*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an obround with height<width and rotation=0°;+/-180° is exported as a
// simple obround aperture.
TEST_F(GerberApertureListTest, testWideObround0deg) {
  GerberApertureList l;
  PositiveLength w(200000);
  PositiveLength h(100000);
  QVector<Angle> rotations = {
      -Angle::deg180(),
      Angle::deg0(),
      Angle::deg180(),
  };

  const char* expected = "%ADD10O,0.2X0.1*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an obround with height>width and rotation=+/-90°;+/-270° is exported
// as a simple obround aperture.
TEST_F(GerberApertureListTest, testHighObround90deg) {
  GerberApertureList l;
  PositiveLength w(100000);
  PositiveLength h(200000);
  QVector<Angle> rotations = {
      -Angle::deg270(),
      -Angle::deg90(),
      Angle::deg90(),
      Angle::deg270(),
  };

  const char* expected = "%ADD10O,0.2X0.1*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an obround with height<width and rotation=+/-90°;+/-270° is exported
// as a simple obround aperture.
TEST_F(GerberApertureListTest, testWideObround90deg) {
  GerberApertureList l;
  PositiveLength w(200000);
  PositiveLength h(100000);
  QVector<Angle> rotations = {
      -Angle::deg270(),
      -Angle::deg90(),
      Angle::deg90(),
      Angle::deg270(),
  };

  const char* expected = "%ADD10O,0.1X0.2*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an obround with height>width and rotation=10°;190°;-170°;-350° is
// exported as a macro which is as simple as possible.
TEST_F(GerberApertureListTest, testHighObround10deg) {
  GerberApertureList l;
  PositiveLength w(100000);
  PositiveLength h(150000);
  QVector<Angle> rotations = {
      Angle(-350000000),
      Angle(-170000000),
      Angle(10000000),
      Angle(190000000),
  };

  // ATTENTION: The circles MUST NOT SPECIFY THE ROTATION!!! This would cause
  // troubles with some CAM software!!!
  const char* expected =
      "%AMROTATEDOBROUND10*"
      "1,1,0.1,0.004341,-0.02462*"
      "1,1,0.1,-0.004341,0.02462*"
      "20,1,0.1,0.004341,-0.02462,-0.004341,0.02462,0*%\n"
      "%ADD10ROTATEDOBROUND10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an obround with height<width and rotation=10°;190°;-170°;-350° is
// exported as a macro which is as simple as possible.
TEST_F(GerberApertureListTest, testWideObround10deg) {
  GerberApertureList l;
  PositiveLength w(150000);
  PositiveLength h(100000);
  QVector<Angle> rotations = {
      Angle(-350000000),
      Angle(-170000000),
      Angle(10000000),
      Angle(190000000),
  };

  // ATTENTION: The circles MUST NOT SPECIFY THE ROTATION!!! This would cause
  // troubles with some CAM software!!!
  const char* expected =
      "%AMROTATEDOBROUND10*"
      "1,1,0.1,-0.02462,-0.004341*"
      "1,1,0.1,0.02462,0.004341*"
      "20,1,0.1,-0.02462,-0.004341,0.02462,0.004341,0*%\n"
      "%ADD10ROTATEDOBROUND10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rect with height>width and rotation=0°;+/-180° is exported as
// a simple rectangular aperture.
TEST_F(GerberApertureListTest, testHighRect0deg) {
  GerberApertureList l;
  PositiveLength w(100000);
  PositiveLength h(150000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      -Angle::deg180(),
      Angle::deg0(),
      Angle::deg180(),
  };

  const char* expected = "%ADD10R,0.1X0.15*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rect with height<width and rotation=0°;+/-180° is exported as
// a simple rectangular aperture.
TEST_F(GerberApertureListTest, testWideRect0deg) {
  GerberApertureList l;
  PositiveLength w(150000);
  PositiveLength h(100000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      -Angle::deg180(),
      Angle::deg0(),
      Angle::deg180(),
  };

  const char* expected = "%ADD10R,0.15X0.1*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rect with height>width and rotation=+/-90°;+/-270° is exported as
// a simple rectangular aperture.
TEST_F(GerberApertureListTest, testHighRect90deg) {
  GerberApertureList l;
  PositiveLength w(100000);
  PositiveLength h(150000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      -Angle::deg270(),
      -Angle::deg90(),
      Angle::deg90(),
      Angle::deg270(),
  };

  const char* expected = "%ADD10R,0.15X0.1*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rect with height<width and rotation=+/-90°;+/-270° is exported as
// a simple rectangular aperture.
TEST_F(GerberApertureListTest, testWideRect90deg) {
  GerberApertureList l;
  PositiveLength w(150000);
  PositiveLength h(100000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      -Angle::deg270(),
      -Angle::deg90(),
      Angle::deg90(),
      Angle::deg270(),
  };

  const char* expected = "%ADD10R,0.1X0.15*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rect with height>width and rotation=10°;190°;-170°;-350° is
// exported as a macro which is as simple as possible.
TEST_F(GerberApertureListTest, testHighRect10deg) {
  GerberApertureList l;
  PositiveLength w(100000);
  PositiveLength h(150000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      Angle(-350000000),
      Angle(-170000000),
      Angle(10000000),
      Angle(190000000),
  };

  // ATTENTION: DO NOT USE THE CENTER LINE (Code 21)!!! It is buggy in some
  // CAM software!!!
  const char* expected =
      "%AMROTATEDRECT10*"
      "20,1,0.1,-0.075,0.0,0.075,0.0,100.0*%\n"
      "%ADD10ROTATEDRECT10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rect with height<width and rotation=10°;190°;-170°;-350° is
// exported as a macro which is as simple as possible.
TEST_F(GerberApertureListTest, testWideRect10deg) {
  GerberApertureList l;
  PositiveLength w(150000);
  PositiveLength h(100000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      Angle(-350000000),
      Angle(-170000000),
      Angle(10000000),
      Angle(190000000),
  };

  // ATTENTION: DO NOT USE THE CENTER LINE (Code 21)!!! It is buggy in some
  // CAM software!!!
  const char* expected =
      "%AMROTATEDRECT10*"
      "20,1,0.1,-0.075,0.0,0.075,0.0,10.0*%\n"
      "%ADD10ROTATEDRECT10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rounded rect with height>width and rotation=10°;190°;-170°;-350°
// is exported as a macro.
TEST_F(GerberApertureListTest, testHighRoundedRect10deg) {
  GerberApertureList l;
  PositiveLength w(100000);
  PositiveLength h(150000);
  UnsignedLength r(20000);
  QVector<Angle> rotations = {
      Angle(-350000000),
      Angle(-170000000),
      Angle(10000000),
      Angle(190000000),
  };

  // ATTENTION: DO NOT USE THE CENTER LINE (Code 21)!!! It is buggy in some
  // CAM software!!!
  const char* expected =
      "%AMROUNDEDRECT10*"
      "20,1,0.1,-0.055,0.0,0.055,0.0,100.0*"
      "20,1,0.06,-0.075,0.0,0.075,0.0,100.0*"
      "1,1,0.04,-0.019994,-0.059374*"
      "1,1,0.04,-0.039095,0.048955*"
      "1,1,0.04,0.019994,0.059374*"
      "1,1,0.04,0.039095,-0.048955*%\n"
      "%ADD10ROUNDEDRECT10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rounded rect with height<width and rotation=10°;190°;-170°;-350°
// is exported as a macro.
TEST_F(GerberApertureListTest, testWideRoundedRect10deg) {
  GerberApertureList l;
  PositiveLength w(150000);
  PositiveLength h(100000);
  UnsignedLength r(20000);
  QVector<Angle> rotations = {
      Angle(-350000000),
      Angle(-170000000),
      Angle(10000000),
      Angle(190000000),
  };

  // ATTENTION: DO NOT USE THE CENTER LINE (Code 21)!!! It is buggy in some
  // CAM software!!!
  const char* expected =
      "%AMROUNDEDRECT10*"
      "20,1,0.1,-0.055,0.0,0.055,0.0,10.0*"
      "20,1,0.06,-0.075,0.0,0.075,0.0,10.0*"
      "1,1,0.04,-0.059374,0.019994*"
      "1,1,0.04,0.048955,0.039095*"
      "1,1,0.04,0.059374,-0.019994*"
      "1,1,0.04,-0.048955,-0.039095*%\n"
      "%ADD10ROUNDEDRECT10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rounded rect with rotations of a multiple of 180° and with a too
// large radius is converted into an obround.
TEST_F(GerberApertureListTest, testObroundRoundedRect0deg) {
  GerberApertureList l;
  PositiveLength w(150000);
  PositiveLength h(100000);
  UnsignedLength r(50000);
  QVector<Angle> rotations = {
      -Angle::deg180(),
      Angle::deg0(),
      Angle::deg180(),
  };

  const char* expected = "%ADD10O,0.15X0.1*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if a rounded rect with a too large radius is converted into an obround.
TEST_F(GerberApertureListTest, testObroundRoundedRect10deg) {
  GerberApertureList l;
  PositiveLength w(150000);
  PositiveLength h(100000);
  UnsignedLength r(60000);
  QVector<Angle> rotations = {
      Angle(-350000000),
      Angle(-170000000),
      Angle(10000000),
      Angle(190000000),
  };

  const char* expected =
      "%AMROTATEDOBROUND10*"
      "1,1,0.1,-0.02462,-0.004341*"
      "1,1,0.1,0.02462,0.004341*"
      "20,1,0.1,-0.02462,-0.004341,0.02462,0.004341,0*%\n"
      "%ADD10ROTATEDOBROUND10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addRect(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(10, l.addObround(w, h, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an octagon with height==width and rotations of a multiple of 45°
// is exported as the same aperture macro.
TEST_F(GerberApertureListTest, testRegularOctagon0deg) {
  GerberApertureList l;
  PositiveLength w(500000);
  PositiveLength h(500000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      -Angle::deg315(), -Angle::deg270(), -Angle::deg225(), -Angle::deg180(),
      -Angle::deg135(), -Angle::deg90(),  -Angle::deg45(),  Angle::deg0(),
      Angle::deg45(),   Angle::deg90(),   Angle::deg135(),  Angle::deg180(),
      Angle::deg225(),  Angle::deg270(),  Angle::deg315(),
  };

  // ATTENTION: DO NOT USE THE REGULAR POLYGON PRIMITIVE (P)!!! It is buggy
  // in some tools!
  const char* expected =
      "%AMROTATEDOCTAGON10*"
      "4,1,8,"
      "0.25,0.103553,"
      "0.103553,0.25,"
      "-0.103553,0.25,"
      "-0.25,0.103553,"
      "-0.25,-0.103553,"
      "-0.103553,-0.25,"
      "0.103553,-0.25,"
      "0.25,-0.103553,"
      "0.25,0.103553,"
      "0.0*%\n"
      "%ADD10ROTATEDOCTAGON10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addOctagon(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an octagon with height==width and rotations of a multiple of 45°
// and an offset of 10° is exported as the same aperture macro.
TEST_F(GerberApertureListTest, testRegularOctagon10deg) {
  GerberApertureList l;
  PositiveLength w(500000);
  PositiveLength h(500000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      Angle(-350000000), Angle(-305000000), Angle(-260000000),
      Angle(-215000000), Angle(-170000000), Angle(-125000000),
      Angle(-80000000),  Angle(-35000000),  Angle(10000000),
      Angle(55000000),   Angle(100000000),  Angle(145000000),
      Angle(190000000),  Angle(235000000),  Angle(280000000),
      Angle(325000000),
  };

  // ATTENTION: DO NOT USE THE REGULAR POLYGON PRIMITIVE (P)!!! It is buggy
  // in some tools!
  const char* expected =
      "%AMROTATEDOCTAGON10*"
      "4,1,8,"
      "0.25,0.103553,"
      "0.103553,0.25,"
      "-0.103553,0.25,"
      "-0.25,0.103553,"
      "-0.25,-0.103553,"
      "-0.103553,-0.25,"
      "0.103553,-0.25,"
      "0.25,-0.103553,"
      "0.25,0.103553,"
      "10.0*%\n"
      "%ADD10ROTATEDOCTAGON10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addOctagon(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an octagon with height>width and rotations of a multiple of 180°
// is exported as the same aperture macro.
TEST_F(GerberApertureListTest, testHighOctagon0deg) {
  GerberApertureList l;
  PositiveLength w(500000);
  PositiveLength h(900000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      -Angle::deg180(),
      Angle::deg0(),
      Angle::deg180(),
  };

  const char* expected =
      "%AMROTATEDOCTAGON10*"
      "4,1,8,"
      "0.45,0.103553,"
      "0.303553,0.25,"
      "-0.303553,0.25,"
      "-0.45,0.103553,"
      "-0.45,-0.103553,"
      "-0.303553,-0.25,"
      "0.303553,-0.25,"
      "0.45,-0.103553,"
      "0.45,0.103553,"
      "90.0*%\n"
      "%ADD10ROTATEDOCTAGON10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addOctagon(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an octagon with height<width and rotations of a multiple of 180°
// is exported as the same aperture macro.
TEST_F(GerberApertureListTest, testWideOctagon0deg) {
  GerberApertureList l;
  PositiveLength w(900000);
  PositiveLength h(500000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      -Angle::deg180(),
      Angle::deg0(),
      Angle::deg180(),
  };

  const char* expected =
      "%AMROTATEDOCTAGON10*"
      "4,1,8,"
      "0.45,0.103553,"
      "0.303553,0.25,"
      "-0.303553,0.25,"
      "-0.45,0.103553,"
      "-0.45,-0.103553,"
      "-0.303553,-0.25,"
      "0.303553,-0.25,"
      "0.45,-0.103553,"
      "0.45,0.103553,"
      "0.0*%\n"
      "%ADD10ROTATEDOCTAGON10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addOctagon(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an octagon with height>width and rotations of a multiple of 180°
// and an offset of 100° is exported as the same aperture macro.
TEST_F(GerberApertureListTest, testHighOctagon100deg) {
  GerberApertureList l;
  PositiveLength w(500000);
  PositiveLength h(900000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      Angle(-260000000),
      Angle(-80000000),
      Angle(100000000),
      Angle(280000000),
  };

  const char* expected =
      "%AMROTATEDOCTAGON10*"
      "4,1,8,"
      "0.45,0.103553,"
      "0.303553,0.25,"
      "-0.303553,0.25,"
      "-0.45,0.103553,"
      "-0.45,-0.103553,"
      "-0.303553,-0.25,"
      "0.303553,-0.25,"
      "0.45,-0.103553,"
      "0.45,0.103553,"
      "10.0*%\n"
      "%ADD10ROTATEDOCTAGON10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addOctagon(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

// Test if an octagon with height<width and rotations of a multiple of 180°
// and an offset of 100° is exported as the same aperture macro.
TEST_F(GerberApertureListTest, testWideOctagon100deg) {
  GerberApertureList l;
  PositiveLength w(900000);
  PositiveLength h(500000);
  UnsignedLength r(0);
  QVector<Angle> rotations = {
      Angle(-260000000),
      Angle(-80000000),
      Angle(100000000),
      Angle(280000000),
  };

  const char* expected =
      "%AMROTATEDOCTAGON10*"
      "4,1,8,"
      "0.45,0.103553,"
      "0.303553,0.25,"
      "-0.303553,0.25,"
      "-0.45,0.103553,"
      "-0.45,-0.103553,"
      "-0.303553,-0.25,"
      "0.303553,-0.25,"
      "0.45,-0.103553,"
      "0.45,0.103553,"
      "100.0*%\n"
      "%ADD10ROTATEDOCTAGON10*%\n";

  for (const Angle& rot : rotations) {
    EXPECT_EQ(10, l.addOctagon(w, h, r, rot, tl::nullopt));
    EXPECT_EQ(expected, l.generateString().toStdString());
  }
}

TEST_F(GerberApertureListTest, testComponentMain) {
  GerberApertureList l;

  // Note: The Gerber specs require exactly this aperture shape!!!
  const char* expected =
      "G04 #@! TA.AperFunction,ComponentMain*\n"
      "%ADD10C,0.3*%\n"
      "G04 #@! TD*\n";

  EXPECT_EQ(10, l.addComponentMain());
  EXPECT_EQ(expected, l.generateString().toStdString());
}

TEST_F(GerberApertureListTest, testComponentPin) {
  GerberApertureList l;

  // Note: The Gerber specs require exactly this aperture shape!!!
  const char* expected =
      "G04 #@! TA.AperFunction,ComponentPin*\n"
      "%ADD10C,0*%\n"
      "%ADD11P,0.36X4X0.0*%\n"
      "G04 #@! TD*\n";

  EXPECT_EQ(10, l.addComponentPin(false));
  EXPECT_EQ(11, l.addComponentPin(true));
  EXPECT_EQ(expected, l.generateString().toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
