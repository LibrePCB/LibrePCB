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
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/utils/transform.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class TransformTest : public ::testing::Test {
protected:
  std::string str(const QString& s) const { return s.toStdString(); }

  std::string str(const Point& p) const {
    return p.serializeToDomElement("pos").toByteArray().toStdString();
  }

  std::string str(const Angle& a) const {
    return a.toDegString().toStdString();
  }

  std::string str(const Path& p) const {
    return p.serializeToDomElement("path").toByteArray().toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(TransformTest, testCopyConstructor) {
  Transform t1(Point(1, 2), Angle(3), true);
  Transform t2(t1);
  EXPECT_EQ(str(t1.getPosition()), str(t2.getPosition()));
  EXPECT_EQ(str(t1.getRotation()), str(t2.getRotation()));
  EXPECT_EQ(t1.getMirrored(), t2.getMirrored());
}

TEST_F(TransformTest, testMapAngleNonMirrored) {
  Transform t(Point(1000, 2000), Angle(3000), false);
  EXPECT_EQ(str(Angle(3000)), str(t.map(Angle(0))));
  EXPECT_EQ(str(Angle(0)), str(t.map(Angle(-3000))));
  EXPECT_EQ(str(Angle(180003000)), str(t.map(Angle(180000000))));
  EXPECT_EQ(str(Angle(-179997000)), str(t.map(Angle(-180000000))));
}

TEST_F(TransformTest, testMapAngleMirrored) {
  Transform t(Point(1000, 2000), Angle(3000), true);
  EXPECT_EQ(str(Angle(179997000)), str(t.map(Angle(0))));
  EXPECT_EQ(str(Angle(180000000)), str(t.map(Angle(-3000))));
  EXPECT_EQ(str(Angle(-3000)), str(t.map(Angle(180000000))));
  EXPECT_EQ(str(Angle(359997000)), str(t.map(Angle(-180000000))));
}

TEST_F(TransformTest, testMapPointNonMirrored) {
  Transform t(Point(1000, 2000), Angle(30000000), false);
  EXPECT_EQ(str(Point(1000, 2000)), str(t.map(Point(0, 0))));
  EXPECT_EQ(str(Point(17, 12836)), str(t.map(Point(4567, 9876))));
}

TEST_F(TransformTest, testMapPointMirrored) {
  Transform t(Point(1000, 2000), Angle(30000000), true);
  EXPECT_EQ(str(Point(1000, 2000)), str(t.map(Point(0, 0))));
  EXPECT_EQ(str(Point(1983, 12836)), str(t.map(Point(4567, 9876))));
}

TEST_F(TransformTest, testMapPathNonMirrored) {
  Transform t(Point(1000, 2000), Angle(30000000), false);
  Path input({
      Vertex(Point(0, 0), Angle::deg90()),
      Vertex(Point(4567, 9876), Angle::deg0()),
  });
  Path expected({
      Vertex(Point(1000, 2000), Angle::deg90()),
      Vertex(Point(17, 12836), Angle::deg0()),
  });
  EXPECT_EQ(str(expected), str(t.map(input)));
}

TEST_F(TransformTest, testMapPathMirrored) {
  Transform t(Point(1000, 2000), Angle(30000000), true);
  Path input({
      Vertex(Point(0, 0), Angle::deg90()),
      Vertex(Point(4567, 9876), Angle::deg0()),
  });
  Path expected({
      Vertex(Point(1000, 2000), -Angle::deg90()),
      Vertex(Point(1983, 12836), Angle::deg0()),
  });
  EXPECT_EQ(str(expected), str(t.map(input)));
}

TEST_F(TransformTest, testMapLayerNonMirrored) {
  Transform t(Point(1000, 2000), Angle(3000), false);
  EXPECT_EQ(str(GraphicsLayer::sSymbolOutlines),
            str(t.map(QString(GraphicsLayer::sSymbolOutlines))));
  EXPECT_EQ(str(GraphicsLayer::sTopCopper),
            str(t.map(QString(GraphicsLayer::sTopCopper))));
  EXPECT_EQ(str(GraphicsLayer::sSymbolOutlines),
            str(*t.map(GraphicsLayerName(GraphicsLayer::sSymbolOutlines))));
  EXPECT_EQ(str(GraphicsLayer::sTopCopper),
            str(*t.map(GraphicsLayerName(GraphicsLayer::sTopCopper))));
}

TEST_F(TransformTest, testMapLayerMirrored) {
  Transform t(Point(1000, 2000), Angle(3000), true);
  EXPECT_EQ(str(GraphicsLayer::sSymbolOutlines),
            str(t.map(QString(GraphicsLayer::sSymbolOutlines))));
  EXPECT_EQ(str(GraphicsLayer::sBotCopper),
            str(t.map(QString(GraphicsLayer::sTopCopper))));
  EXPECT_EQ(str(GraphicsLayer::sSymbolOutlines),
            str(*t.map(GraphicsLayerName(GraphicsLayer::sSymbolOutlines))));
  EXPECT_EQ(str(GraphicsLayer::sBotCopper),
            str(*t.map(GraphicsLayerName(GraphicsLayer::sTopCopper))));
}

TEST_F(TransformTest, testOperatorAssign) {
  Transform t1(Point(1, 2), Angle(3), true);
  Transform t2(Point(0, 0), Angle(0), false);
  t2 = t1;
  EXPECT_EQ(str(t1.getPosition()), str(t2.getPosition()));
  EXPECT_EQ(str(t1.getRotation()), str(t2.getRotation()));
  EXPECT_EQ(t1.getMirrored(), t2.getMirrored());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
