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
#include <librepcb/common/geometry/pathmodel.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class PathModelTest : public ::testing::Test {
protected:
  static Path createPopulatedPath() noexcept {
    Path path;
    path.addVertex(Point(1, 2), Angle(3));
    path.addVertex(Point(0, 0), Angle(0));
    path.addVertex(Point(1000, 2000), Angle(3000));
    return path;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(PathModelTest, testData) {
  PathModel model(nullptr);
  model.setPath(createPopulatedPath());
  EXPECT_EQ(QVariant::fromValue(Length(1000)),
            model.data(model.index(2, PathModel::COLUMN_X)));
  EXPECT_EQ(QVariant::fromValue(Length(2000)),
            model.data(model.index(2, PathModel::COLUMN_Y)));
  EXPECT_EQ(QVariant::fromValue(Angle(3)),
            model.data(model.index(0, PathModel::COLUMN_ANGLE)));
}

TEST_F(PathModelTest, testSetData) {
  PathModel model(nullptr);
  model.setPath(createPopulatedPath());
  bool r = model.setData(model.index(1, PathModel::COLUMN_X),
                         QVariant::fromValue(Length(5080000)));
  EXPECT_TRUE(r);
  r = model.setData(model.index(1, PathModel::COLUMN_Y),
                    QVariant::fromValue(Length(1234568)));
  EXPECT_TRUE(r);
  r = model.setData(model.index(1, PathModel::COLUMN_ANGLE),
                    QVariant::fromValue(Angle(45000000)));
  EXPECT_TRUE(r);

  Path expected             = createPopulatedPath();
  expected.getVertices()[1] = Vertex(Point(5080000, 1234568), Angle(45000000));
  EXPECT_EQ(expected, model.getPath());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
