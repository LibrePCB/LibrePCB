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
#include <librepcb/editor/spacemouse/spacemousemotionmapper.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(SpaceMouseMotionMapperTest, testAllZeroIsNoOp) {
  const SpaceMouseMotionEvent e;
  const SpaceMouseMotion2d motion = toSpaceMouseMotion2d(e, 1.0);
  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
}

TEST(SpaceMouseMotionMapperTest, testZeroDtIsNoOpRegardlessOfDeflection) {
  // No elapsed time must mean no motion at all, no matter how hard the
  // device is deflected - this is what keeps the result rate-independent
  // (see the doc comment on toSpaceMouseMotion2d()).
  SpaceMouseMotionEvent e;
  e.translationX = 350;
  e.translationY = 350;
  e.translationZ = 350;
  const SpaceMouseMotion2d motion = toSpaceMouseMotion2d(e, 0.0);
  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
}

TEST(SpaceMouseMotionMapperTest, testTranslationXyMapsToPan) {
  SpaceMouseMotionEvent e;
  e.translationX = 100;
  e.translationY = 50;
  const SpaceMouseMotion2d motion = toSpaceMouseMotion2d(e, 1.0);

  // Both X and Y are inverted (tuned against real hardware, see the doc
  // comment on toSpaceMouseMotion2d() - X/Z were correct on the first
  // hardware test, Y was flipped after a follow-up test), both scaled by
  // the same sensitivity - so the ratio between them stays 2:1 regardless.
  EXPECT_LT(motion.panDelta.x(), 0);
  EXPECT_LT(motion.panDelta.y(), 0);
  EXPECT_DOUBLE_EQ(motion.panDelta.x(), 2 * motion.panDelta.y());
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);  // Unaffected by translationX/Y.
}

TEST(SpaceMouseMotionMapperTest, testPanScalesLinearlyWithDt) {
  SpaceMouseMotionEvent e;
  e.translationX = 100;
  e.translationY = 50;
  const SpaceMouseMotion2d motionAt1s = toSpaceMouseMotion2d(e, 1.0);
  const SpaceMouseMotion2d motionAt2s = toSpaceMouseMotion2d(e, 2.0);

  // Twice the elapsed time (at the same held deflection) must mean exactly
  // twice the pan distance - this is the whole point of taking dtSeconds
  // as a parameter instead of applying a fixed step per raw HID report.
  EXPECT_DOUBLE_EQ(2 * motionAt1s.panDelta.x(), motionAt2s.panDelta.x());
  EXPECT_DOUBLE_EQ(2 * motionAt1s.panDelta.y(), motionAt2s.panDelta.y());
}

TEST(SpaceMouseMotionMapperTest, testTranslationZMapsToZoom) {
  SpaceMouseMotionEvent positiveZ;
  positiveZ.translationZ = 200;
  SpaceMouseMotionEvent negativeZ;
  negativeZ.translationZ = -200;

  const SpaceMouseMotion2d positiveZMotion = toSpaceMouseMotion2d(positiveZ, 1.0);
  const SpaceMouseMotion2d negativeZMotion = toSpaceMouseMotion2d(negativeZ, 1.0);

  EXPECT_EQ(QPointF(0, 0), positiveZMotion.panDelta);  // Unaffected by Z.
  EXPECT_LT(positiveZMotion.zoomFactor, 1.0);
  EXPECT_GT(negativeZMotion.zoomFactor, 1.0);

  // Symmetric deflection should give a reciprocal zoom factor.
  EXPECT_NEAR(1.0, positiveZMotion.zoomFactor * negativeZMotion.zoomFactor,
             1e-9);
}

TEST(SpaceMouseMotionMapperTest, testRotationIsIgnored) {
  SpaceMouseMotionEvent e;
  e.rotationX = 300;
  e.rotationY = -300;
  e.rotationZ = 150;
  const SpaceMouseMotion2d motion = toSpaceMouseMotion2d(e, 1.0);

  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
