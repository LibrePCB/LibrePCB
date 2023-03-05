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
#include <librepcb/core/algorithm/airwiresbuilder.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AirWiresBuilderTest : public ::testing::Test {
protected:
  static AirWiresBuilder::AirWires sorted(
      AirWiresBuilder::AirWires airwires) noexcept {
    for (AirWiresBuilder::AirWire& airwire : airwires) {
      if (airwire.second < airwire.first) {
        std::swap(airwire.second, airwire.first);
      }
    }
    std::sort(airwires.begin(), airwires.end());
    return airwires;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(AirWiresBuilderTest, testEmpty) {
  AirWiresBuilder builder;
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  EXPECT_EQ(0, airwires.size());
}

TEST_F(AirWiresBuilderTest, testOnePoint) {
  AirWiresBuilder builder;
  builder.addPoint(Point(1000000, 2000000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  EXPECT_EQ(0, airwires.size());
}

TEST_F(AirWiresBuilderTest, testTwoUnconnectedPoints) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(1000000, 2000000));
  const int id1 = builder.addPoint(Point(3000000, 4000000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{id0, id1}};
  EXPECT_EQ(expected, airwires);
}

TEST_F(AirWiresBuilderTest, testTwoUnconnectedOverlappingPoints) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(100000, 200000));
  const int id1 = builder.addPoint(Point(100000, 200000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{id0, id1}};
  EXPECT_EQ(expected, airwires);
}

TEST_F(AirWiresBuilderTest, testTwoConnectedPoints) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(100000, 200000));
  const int id1 = builder.addPoint(Point(300000, 400000));
  builder.addEdge(id0, id1);
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  EXPECT_EQ(0, airwires.size());
}

TEST_F(AirWiresBuilderTest, testThreeUnconnectedPoints) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(100000, 200000));
  const int id1 = builder.addPoint(Point(300000, 400000));
  const int id2 = builder.addPoint(Point(-50000, -50000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {
      {id0, id1},
      {id0, id2},
  };
  EXPECT_EQ(expected, airwires);
}

TEST_F(AirWiresBuilderTest, testThreeUnconnectedPointsVshape) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(-50000, 0));
  const int id1 = builder.addPoint(Point(100000, 0));
  const int id2 = builder.addPoint(Point(0, -1000000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {
      {id0, id1},
      {id0, id2},
  };
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testThreeUnconnectedColinearPoints) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(0, 0));
  const int id1 = builder.addPoint(Point(100000, 0));
  const int id2 = builder.addPoint(Point(-100000, 0));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {
      {id0, id1},
      {id0, id2},
  };
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testThreeUnconnectedDiagonalColinearPoints) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(0, 0));
  const int id1 = builder.addPoint(Point(1000000, 1000000));
  const int id2 = builder.addPoint(Point(2000000, 2000000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {
      {id0, id1},
      {id1, id2},
  };
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testThreeUnconnectedDiagonalColinearPoints2) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(71437500, 78898800));
  const int id1 = builder.addPoint(Point(70485000, 80010000));
  const int id2 = builder.addPoint(Point(72707500, 77470000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {
      {id0, id1},
      {id0, id2},
  };
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testPartlyConnectedColinearPoints) {
  AirWiresBuilder builder;
  const int id0 = builder.addPoint(Point(0, 0));
  const int id1 = builder.addPoint(Point(100000, 100000));
  const int id2 = builder.addPoint(Point(200000, 200000));
  const int id3 = builder.addPoint(Point(300000, 300000));
  const int id4 = builder.addPoint(Point(400000, 400000));
  const int id5 = builder.addPoint(Point(500000, 500000));
  const int id6 = builder.addPoint(Point(600000, 600000));
  builder.addEdge(id1, id2);
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {
      {id0, id1}, {id2, id3}, {id3, id4}, {id4, id5}, {id5, id6},
  };
  EXPECT_EQ(expected, airwires);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
