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
#include <librepcb/core/export/gerbergenerator.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/geometry/path.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GerberGeneratorTest : public ::testing::Test {
protected:
  /**
   * @brief Helper function to generate output containing all possible features
   *
   * @return Gerber content
   */
  static const QString& generateEverything() noexcept {
    static QString s;
    if (s.isEmpty()) {
      GerberGenerator gen(
          QDateTime(QDate(2000, 2, 1), QTime(1, 2, 3, 4), Qt::OffsetFromUTC,
                    3600),
          "Project Name",
          Uuid::fromString("bdf7bea5-b88e-41b2-be85-c1604e8ddfca"), "rev-1.0");

      gen.setFileFunctionCopper(1, GerberGenerator::CopperSide::Top,
                                GerberGenerator::Polarity::Positive);

      gen.setLayerPolarity(GerberGenerator::Polarity::Negative);
      gen.setLayerPolarity(GerberGenerator::Polarity::Positive);

      QVector<GerberApertureList::Function> functions = {
          tl::nullopt,
          tl::make_optional(GerberAttribute::ApertureFunction::Conductor),
      };
      QVector<tl::optional<QString>> nets = {tl::optional<QString>(),
                                             tl::make_optional(QString()),
                                             tl::make_optional(QString("Foo"))};
      QStringList components = {"", "Bar"};
      QStringList pins = {"", "42"};
      QStringList pinSignals = {"", "ENABLE"};

      for (const auto& function : functions) {
        for (const auto& net : nets) {
          for (const auto& component : components) {
            gen.drawLine(Point(500, 600), Point(700, 800),
                         UnsignedLength(100000), function, net, component);

            gen.drawPathOutline(Path::circle(PositiveLength(1000000)),
                                UnsignedLength(100000), function, net,
                                component);
            gen.drawPathOutline(Path::centeredRect(PositiveLength(1000000),
                                                   PositiveLength(1000000)),
                                UnsignedLength(100000), function, net,
                                component);

            gen.drawPathArea(Path::circle(PositiveLength(1000000)), function,
                             net, component);
            gen.drawPathArea(Path::centeredRect(PositiveLength(1000000),
                                                PositiveLength(1000000)),
                             function, net, component);

            for (const auto& pin : pins) {
              for (const auto& signal : pinSignals) {
                gen.flashCircle(Point(100, 200), PositiveLength(100000),
                                function, net, component, pin, signal);

                for (int i = -355; i <= 355; i += 5) {
                  Angle rot = Angle(i * 1000000);  // -355..+355° in 5° steps

                  gen.flashRect(Point(100, 200), PositiveLength(100000),
                                PositiveLength(100000), rot, function, net,
                                component, pin, signal);
                  gen.flashRect(Point(100, 200), PositiveLength(100000),
                                PositiveLength(200000), rot, function, net,
                                component, pin, signal);
                  gen.flashRect(Point(100, 200), PositiveLength(200000),
                                PositiveLength(100000), rot, function, net,
                                component, pin, signal);

                  gen.flashObround(Point(100, 200), PositiveLength(100000),
                                   PositiveLength(100000), rot, function, net,
                                   component, pin, signal);
                  gen.flashObround(Point(100, 200), PositiveLength(100000),
                                   PositiveLength(200000), rot, function, net,
                                   component, pin, signal);
                  gen.flashObround(Point(100, 200), PositiveLength(200000),
                                   PositiveLength(100000), rot, function, net,
                                   component, pin, signal);

                  gen.flashOctagon(Point(100, 200), PositiveLength(100000),
                                   PositiveLength(100000), rot, function, net,
                                   component, pin, signal);
                  gen.flashOctagon(Point(100, 200), PositiveLength(100000),
                                   PositiveLength(200000), rot, function, net,
                                   component, pin, signal);
                  gen.flashOctagon(Point(100, 200), PositiveLength(200000),
                                   PositiveLength(100000), rot, function, net,
                                   component, pin, signal);
                }
              }
            }
          }
        }
      }
      gen.generate();
      s = gen.toStr();
    }
    return s;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

// Check if there are no X2 attributes if not explicitly enabled.
TEST_F(GerberGeneratorTest, testDoesNotContainX2Attributes) {
  QString s = generateEverything();
  EXPECT_FALSE(s.contains("%T"));
}

// Check if we always use multi quadrant mode (G75) and never single quadrant
// mode (G74). G74 is buggy in some renderers (see
// https://github.com/LibrePCB/LibrePCB/issues/247) and was marked as deprecated
// in the current Gerber specs.
TEST_F(GerberGeneratorTest, testUsingOnlyMultiQuadrantMode) {
  QString s = generateEverything();
  EXPECT_FALSE(s.contains("G74"));
  EXPECT_TRUE(s.contains("G75"));
}

// Check if there are no zero-sized apertures used. Such apertures are generally
// allowed, but not recommended by the Gerber specs. We even already had some
// issues with such apertures in the past. Since only circles are allowed to
// have a size of zero, we only need to check all circle apertures.
TEST_F(GerberGeneratorTest, testDoesNotContainZeroSizeApertures) {
  int checkedCircles = 0;
  QString s = generateEverything();
  foreach (const QString line, s.split('\n')) {
    QRegularExpression re("^%ADD\\d+C,(.+)\\*%$");
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
      EXPECT_GT(match.captured(1).toDouble(), 0.0);
      ++checkedCircles;
    }
  }
  ASSERT_GE(checkedCircles, 3);  // Sanity check if test works.
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
