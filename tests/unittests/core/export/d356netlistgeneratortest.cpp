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
#include <librepcb/core/export/d356netlistgenerator.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

static const QString sHeader =
    "C  IPC-D-356A Netlist\n"
    "C  \n"
    "C  Project Name:        My Project\n"
    "C  Project Version:     1.0\n"
    "C  Board Name:          My Board\n"
    "C  Generation Software: LibrePCB 0.1.2\n"
    "C  Generation Date:     2019-01-02T03:04:05+01:00\n"
    "C  \n"
    "C  Note that due to limitations of this file format, LibrePCB\n"
    "C  applies the following operations during the export:\n"
    "C    - suffix net names with unique numbers within braces\n"
    "C    - truncate long net names (uniqueness guaranteed by suffix)\n"
    "C    - truncate long component names (uniqueness not guaranteed)\n"
    "C    - truncate long pad names (uniqueness not guaranteed)\n"
    "C    - clip drill/pad sizes to 9.999mm\n"
    "C  \n"
    "P  UNITS CUST 1\n";

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class D356NetlistGeneratorTest : public ::testing::Test {
protected:
  std::string makeComparable(QString str) const noexcept {
    // Replace volatile data in exported file with well-known, constant data.
    str.replace(QRegularExpression("Generation Software: LibrePCB (.*)"),
                "Generation Software: LibrePCB 0.1.2");
    return str.toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(D356NetlistGeneratorTest, testSmtPad) {
  D356NetlistGenerator gen(
      "My Project", "1.0", "My Board",
      QDateTime(QDate(2019, 1, 2), QTime(3, 4, 5, 6), Qt::OffsetFromUTC, 3600));

  gen.smtPad(QString(), QString(), QString(), Point(1111, -2222),
             PositiveLength(123456), PositiveLength(654321), Angle::deg0(), 1);
  gen.smtPad("N/C", "U1", "42", Point(-11111, 22222), PositiveLength(234567),
             PositiveLength(765432), Angle::deg90(), 1);
  gen.smtPad("N/C", "U2", "1337", Point(-11111, 22222), PositiveLength(234567),
             PositiveLength(765432), -Angle::deg90(), 5);
  gen.smtPad("TooooLooogName", "AlsoTooLong", "AsWell", Point(55555, -66666),
             PositiveLength(20000000), PositiveLength(30000000),
             -Angle::deg180(), 5);

  const QString expected = sHeader %  // clang-format off
  "327N/C              NOREF -NPAD       A01X+000001Y-000002X0123Y0654R000 S2\n"
  "327N/C{2}           U1    -42         A01X-000011Y+000022X0235Y0765R090 S2\n"
  "327N/C{2}           U2    -1337       A05X-000011Y+000022X0235Y0765R270 S1\n"
  "327TooooLooogN{3}   AlsoTo-AsWe       A05X+000056Y-000067X9999Y9999R180 S1\n"
  "999\n";  // clang-format on
  EXPECT_EQ(expected.toStdString(), makeComparable(gen.generate()));
}

TEST_F(D356NetlistGeneratorTest, testThtPad) {
  D356NetlistGenerator gen(
      "My Project", "1.0", "My Board",
      QDateTime(QDate(2019, 1, 2), QTime(3, 4, 5, 6), Qt::OffsetFromUTC, 3600));

  gen.thtPad(QString(), QString(), QString(), Point(1111, -2222),
             PositiveLength(123456), PositiveLength(654321), Angle::deg0(),
             PositiveLength(1300000));
  gen.thtPad("N/C", "U1", "42", Point(-11111, 22222), PositiveLength(234567),
             PositiveLength(765432), Angle::deg90(), PositiveLength(444444));
  gen.thtPad("N/C", "U2", "1337", Point(-11111, 22222), PositiveLength(234567),
             PositiveLength(765432), -Angle::deg90(), PositiveLength(555555));
  gen.thtPad("TooooLooogName", "AlsoTooLong", "AsWell", Point(55555, -66666),
             PositiveLength(20000000), PositiveLength(30000000),
             -Angle::deg180(), PositiveLength(20000000));

  const QString expected = sHeader %  // clang-format off
  "317N/C              NOREF -NPAD D1300PA00X+000001Y-000002X0123Y0654R000 S0\n"
  "317N/C{2}           U1    -42   D0444PA00X-000011Y+000022X0235Y0765R090 S0\n"
  "317N/C{2}           U2    -1337 D0556PA00X-000011Y+000022X0235Y0765R270 S0\n"
  "317TooooLooogN{3}   AlsoTo-AsWe D9999PA00X+000056Y-000067X9999Y9999R180 S0\n"
  "999\n";  // clang-format on
  EXPECT_EQ(expected.toStdString(), makeComparable(gen.generate()));
}

TEST_F(D356NetlistGeneratorTest, testVia) {
  D356NetlistGenerator gen(
      "My Project", "1.0", "My Board",
      QDateTime(QDate(2019, 1, 2), QTime(3, 4, 5, 6), Qt::OffsetFromUTC, 3600));

  gen.throughVia(QString(), Point(1111, -2222), PositiveLength(123456),
                 PositiveLength(654321), Angle::deg0(), PositiveLength(1300000),
                 false);
  gen.blindVia("N/C", Point(-11111, 22222), PositiveLength(234567),
               PositiveLength(765432), Angle::deg90(), PositiveLength(444444),
               1, 3, false);
  gen.blindVia("N/C", Point(-11111, 22222), PositiveLength(234567),
               PositiveLength(765432), -Angle::deg90(), PositiveLength(555555),
               3, 64, true);
  gen.buriedVia("TooooLooogName", Point(55555, -66666),
                PositiveLength(20000000), 5, 7);

  // Note: Not sure if blind and buried vias are represented correctly, we
  // need specs which are more clear!
  const QString expected = sHeader %  // clang-format off
  "317N/C              VIA        MD1300PA00X+000001Y-000002X0123Y0654R000 S0\n"
  "307N/C{2}           VIA        MD0444PA01X-000011Y+000022               S2L01L03\n"
  "027                 VIA               A01X-000011Y+000022X0235Y0765R090\n"
  "307N/C{2}           VIA        MD0556PA64X-000011Y+000022               S3L03L64\n"
  "027                 VIA               A64X-000011Y+000022X0235Y0765R270\n"
  "307TooooLooogN{3}   VIA        MD9999P   X+000056Y-000067               S3L05L07\n"
  "999\n";  // clang-format on
  EXPECT_EQ(expected.toStdString(), makeComparable(gen.generate()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
