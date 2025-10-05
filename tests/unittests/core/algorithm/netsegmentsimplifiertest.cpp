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
#include <librepcb/core/algorithm/netsegmentsimplifier.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class NetSegmentSimplifierTest : public ::testing::Test {
protected:
  using AnchorType = NetSegmentSimplifier::AnchorType;
  using Line = NetSegmentSimplifier::Line;
  using Result = NetSegmentSimplifier::Result;

  static std::string str(const Result& result) noexcept {
    QStringList s;
    for (const auto& line : result.lines) {
      s.append(QString("line id=%1 p1=%2 p2=%3 layer=%4 width=%5 modified=%6")
                   .arg(line.id)
                   .arg(line.p1)
                   .arg(line.p2)
                   .arg(line.layer ? line.layer->getId() : "nullptr")
                   .arg(line.width.toMmString())
                   .arg(line.modified ? "true" : "false"));
    }
    for (auto it = result.newJunctions.begin(); it != result.newJunctions.end();
         it++) {
      s.append(QString("new junction id=%1 x=%2 y=%3")
                   .arg(it.key())
                   .arg(it->getX().toMmString())
                   .arg(it->getY().toMmString()));
    }
    for (int id : Toolbox::sortedQSet(result.disconnectedPinsOrPads)) {
      s.append(QString("disconnected pin/pad id=%1").arg(id));
    }
    s.append(QString("modified=%1").arg(result.modified ? "true" : "false"));
    return s.join("\n").toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(NetSegmentSimplifierTest, testEmpty) {
  NetSegmentSimplifier obj;
  const Result actual = obj.simplify();

  const Result expected{{}, {}, {}, false};
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testIncrementingIdsAndResetState) {
  NetSegmentSimplifier obj;
  for (int i = 0; i < 2; ++i) {
    const int p0 =
        obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
    EXPECT_EQ(0, p0);
    const int p1 =
        obj.addAnchor(AnchorType::Via, Point(1000, 1000), nullptr, nullptr);
    EXPECT_EQ(1, p1);
    const int p2 =
        obj.addAnchor(AnchorType::Via, Point(1000, 1000), nullptr, nullptr);
    EXPECT_EQ(2, p2);
    const int l0 = obj.addLine(p0, p1, nullptr, Length(1));
    EXPECT_EQ(0, l0);
    const int l1 = obj.addLine(p1, p2, nullptr, Length(1));
    EXPECT_EQ(1, l1);
    obj.simplify();  // Must reset the state, i.e. reuse IDs.
  }
}

TEST_F(NetSegmentSimplifierTest, testOnlyAnchors) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Via, Point(1000, 1000), nullptr, nullptr);
  const Result actual = obj.simplify();

  const Result expected{{}, {}, {}, false};
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testOneLine) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Via, Point(1000, 1000), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  const Result actual = obj.simplify();

  const Result expected{
      {Line{0, 0, 1, nullptr, Length(1), false}}, {}, {}, false};
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testDuplicateJunctions) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(10, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(10, 10), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(-10, 0), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  obj.addLine(1, 2, nullptr, Length(2));
  obj.addLine(2, 3, nullptr, Length(3));
  obj.addLine(3, 4, nullptr, Length(4));
  const Result actual = obj.simplify();

  const Result expected{{
                            Line{0, 0, 1, nullptr, Length(1), false},
                            Line{1, 1, 2, nullptr, Length(2), false},
                            Line{2, 2, 0, nullptr, Length(3), true},
                            Line{3, 0, 4, nullptr, Length(4), true},
                        },
                        {},
                        {},
                        true};
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testTwoRedundantLines) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 1000), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  obj.addLine(1, 0, nullptr, Length(2));
  const Result actual = obj.simplify();

  const Result expected{
      {Line{1, 1, 0, nullptr, Length(2), false}}, {}, {}, true};
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testOneZeroLengthLineBetweenJunctions) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  const Result actual = obj.simplify();

  const Result expected{{}, {}, {}, true};
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testKeepZeroLengthLineBetweenPins) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::PinOrPad, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::PinOrPad, Point(0, 0), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  const Result actual = obj.simplify();

  const Result expected{
      {Line{0, 0, 1, nullptr, Length(1), false}}, {}, {}, false};
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testMergeStraightLines) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(2000, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(3000, 100), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  obj.addLine(1, 2, nullptr, Length(1));
  obj.addLine(2, 3, nullptr, Length(3));
  const Result actual = obj.simplify();

  const Result expected{
      {
          Line{0, 0, 2, nullptr, Length(1), true},
          Line{2, 2, 3, nullptr, Length(3), false},
      },
      {},
      {},
      true,
  };
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testKeepStraightLinesWithDifferentWidth) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(2000, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(3000, 100), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  obj.addLine(1, 2, nullptr, Length(2));  // different width
  obj.addLine(2, 3, nullptr, Length(3));
  const Result actual = obj.simplify();

  const Result expected{
      {
          Line{0, 0, 1, nullptr, Length(1), false},
          Line{1, 1, 2, nullptr, Length(2), false},
          Line{2, 2, 3, nullptr, Length(3), false},
      },
      {},
      {},
      false,
  };
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testSplitLineAtExistingAnchor) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(200, 0), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  obj.addLine(1, 2, nullptr, Length(2));
  obj.addLine(2, 3, nullptr, Length(3));
  const Result actual = obj.simplify();

  const Result expected{
      {
          Line{0, 0, 3, nullptr, Length(1), true},
          Line{1, 1, 2, nullptr, Length(2), false},
          Line{2, 2, 3, nullptr, Length(3), false},
          Line{3, 3, 1, nullptr, Length(1), true},  // new
      },
      {},
      {},
      true,
  };
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testSplitIntersectingLines) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(700, 1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(700, -1000), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  obj.addLine(1, 2, nullptr, Length(2));
  obj.addLine(2, 3, nullptr, Length(3));
  const Result actual = obj.simplify();

  const Result expected{
      {
          Line{0, 0, 4, nullptr, Length(1), true},  // split
          Line{1, 1, 2, nullptr, Length(2), false},
          Line{2, 2, 4, nullptr, Length(3), true},  // split
          Line{3, 4, 1, nullptr, Length(1), true},  // new
          Line{4, 4, 3, nullptr, Length(3), true},  // new
      },
      {
          {4, Point(700, 0)},
      },
      {},
      true,
  };
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testSplitMultipleIntersectingLines) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(1000, 1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(800, 1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(800, -1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(600, -1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(600, 1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(400, 1000), nullptr, nullptr);
  obj.addAnchor(AnchorType::Junction, Point(400, -1000), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  obj.addLine(1, 2, nullptr, Length(1));
  obj.addLine(2, 3, nullptr, Length(1));
  obj.addLine(3, 4, nullptr, Length(1));
  obj.addLine(4, 5, nullptr, Length(1));
  obj.addLine(5, 6, nullptr, Length(1));
  obj.addLine(6, 7, nullptr, Length(1));
  obj.addLine(7, 8, nullptr, Length(1));
  const Result actual = obj.simplify();

  const Result expected{
      {
          Line{0, 0, 11, nullptr, Length(1), true},  // split
          Line{1, 1, 2, nullptr, Length(1), false},
          Line{2, 2, 3, nullptr, Length(1), false},
          Line{3, 3, 9, nullptr, Length(1), true},  // split
          Line{4, 4, 5, nullptr, Length(1), false},
          Line{5, 5, 10, nullptr, Length(1), true},  // split
          Line{6, 6, 7, nullptr, Length(1), false},
          Line{7, 7, 11, nullptr, Length(1), true},  // split
          Line{8, 9, 1, nullptr, Length(1), true},  // new
          Line{9, 10, 9, nullptr, Length(1), true},  // new
          Line{10, 11, 10, nullptr, Length(1), true},  // new
          Line{11, 9, 4, nullptr, Length(1), true},  // new
          Line{12, 10, 6, nullptr, Length(1), true},  // new
          Line{13, 11, 8, nullptr, Length(1), true},  // new
      },
      {
          {9, Point(800, 0)},
          {10, Point(600, 0)},
          {11, Point(400, 0)},
      },
      {},
      true,
  };
  EXPECT_EQ(str(expected), str(actual));
}

TEST_F(NetSegmentSimplifierTest, testDisconnectedPinsOrPads) {
  NetSegmentSimplifier obj;
  obj.addAnchor(AnchorType::Junction, Point(0, 0), nullptr, nullptr);
  obj.addAnchor(AnchorType::PinOrPad, Point(0, 0), nullptr, nullptr);
  obj.addLine(0, 1, nullptr, Length(1));
  const Result actual = obj.simplify();

  const Result expected{
      {},  // Line removed
      {},  // No new junctions
      {1},  // Pin 1 disconnected
      true,
  };
  EXPECT_EQ(str(expected), str(actual));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
