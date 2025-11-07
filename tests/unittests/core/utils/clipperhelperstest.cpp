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
#include <librepcb/core/utils/clipperhelpers.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ClipperHelpersTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

// Test to reproduce https://github.com/LibrePCB/LibrePCB/issues/974
TEST_F(ClipperHelpersTest, testConvertPathWithApproximate) {
  Path input({Vertex(Point(30875000, 32385000), -Angle::deg180()),
              Vertex(Point(26275000, 32385000), -Angle::deg180()),
              Vertex(Point(30875000, 32385000), Angle::deg0())});
  PositiveLength maxArcTolerance(1000000);
  Clipper2Lib::Path64 output = ClipperHelpers::convert(input, maxArcTolerance);

  QString outputStr;
  for (const auto& p : output) {
    outputStr += QString("(%1, %2) ").arg(p.x).arg(p.y);
  }
  EXPECT_EQ(
      "(30875000, 32385000) (29725000, 34376858) (27425000, 34376858) "
      "(26275000, 32385000) (27425000, 30393142) (29725000, 30393142) "
      "(30875000, 32385000) ",
      outputStr.toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
