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

#include <limits>

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
  const SpaceMouseMotion2d motion = SpaceMouseMotionMapper::toMotion2d(e, 1.0);
  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
}

TEST(SpaceMouseMotionMapperTest, testZeroDtIsNoOpRegardlessOfDeflection) {
  // No elapsed time must mean no motion at all, no matter how hard the
  // device is deflected - this is what keeps the result rate-independent
  // (see the doc comment on SpaceMouseMotionMapper::toMotion2d()).
  SpaceMouseMotionEvent e;
  e.translationX = 350;
  e.translationY = 350;
  e.translationZ = 350;
  const SpaceMouseMotion2d motion = SpaceMouseMotionMapper::toMotion2d(e, 0.0);
  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
}

TEST(SpaceMouseMotionMapperTest, testTranslationXyMapsToPan) {
  SpaceMouseMotionEvent e;
  e.translationX = 100;
  e.translationY = 50;
  const SpaceMouseMotion2d motion = SpaceMouseMotionMapper::toMotion2d(e, 1.0);

  // Both X and Y are inverted (tuned against real hardware), both scaled by
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
  const SpaceMouseMotion2d motionAt1s = SpaceMouseMotionMapper::toMotion2d(e, 1.0);
  const SpaceMouseMotion2d motionAt2s = SpaceMouseMotionMapper::toMotion2d(e, 2.0);

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

  const SpaceMouseMotion2d positiveZMotion =
      SpaceMouseMotionMapper::toMotion2d(positiveZ, 1.0);
  const SpaceMouseMotion2d negativeZMotion =
      SpaceMouseMotionMapper::toMotion2d(negativeZ, 1.0);

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
  const SpaceMouseMotion2d motion = SpaceMouseMotionMapper::toMotion2d(e, 1.0);

  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
}

TEST(SpaceMouseMotionMapperTest, testDeflectionIsSaturated) {
  // Anything beyond the saturation value must not move any faster.
  SpaceMouseMotionEvent atSaturation;
  atSaturation.translationX = qRound(SpaceMouseMotionMapper::sAxisSaturation);
  SpaceMouseMotionEvent beyondSaturation;
  beyondSaturation.translationX = 30000;
  EXPECT_EQ(SpaceMouseMotionMapper::toMotion2d(atSaturation, 1.0).panDelta,
            SpaceMouseMotionMapper::toMotion2d(beyondSaturation, 1.0).panDelta);
}

/*******************************************************************************
 *  Test Methods: SpaceMouseMotionMapper::toMotion3d()
 ******************************************************************************/

// The axis signs of the 3D mapping were tuned against real hardware, so pin
// them here to catch accidental changes.
TEST(SpaceMouseMotionMapperTest, test3dAllZeroIsNoOp) {
  const SpaceMouseMotion3d motion =
      SpaceMouseMotionMapper::toMotion3d(SpaceMouseMotionEvent(), 1.0);
  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
  EXPECT_DOUBLE_EQ(0.0, motion.rotateXDeg);
  EXPECT_DOUBLE_EQ(0.0, motion.rotateYDeg);
  EXPECT_DOUBLE_EQ(0.0, motion.rotateZDeg);
}

TEST(SpaceMouseMotionMapperTest, test3dZeroDtIsNoOp) {
  SpaceMouseMotionEvent e;
  e.translationX = 350;
  e.translationY = 350;
  e.translationZ = 350;
  e.rotationX = 350;
  e.rotationY = 350;
  e.rotationZ = 350;
  const SpaceMouseMotion3d motion = SpaceMouseMotionMapper::toMotion3d(e, 0.0);
  EXPECT_EQ(QPointF(0, 0), motion.panDelta);
  EXPECT_DOUBLE_EQ(1.0, motion.zoomFactor);
  EXPECT_DOUBLE_EQ(0.0, motion.rotateXDeg);
  EXPECT_DOUBLE_EQ(0.0, motion.rotateYDeg);
  EXPECT_DOUBLE_EQ(0.0, motion.rotateZDeg);
}

TEST(SpaceMouseMotionMapperTest, test3dAxisSigns) {
  SpaceMouseMotionEvent e;
  e.translationX = 100;
  e.translationY = 100;
  e.translationZ = 100;
  e.rotationX = 100;
  e.rotationY = 100;
  e.rotationZ = 100;
  const SpaceMouseMotion3d motion = SpaceMouseMotionMapper::toMotion3d(e, 1.0);
  EXPECT_GT(motion.panDelta.x(), 0);
  EXPECT_LT(motion.panDelta.y(), 0);
  EXPECT_LT(motion.zoomFactor, 1.0);
  EXPECT_GT(motion.rotateXDeg, 0);
  EXPECT_LT(motion.rotateYDeg, 0);
  EXPECT_LT(motion.rotateZDeg, 0);
}

TEST(SpaceMouseMotionMapperTest, test3dScalesLinearlyWithDt) {
  SpaceMouseMotionEvent e;
  e.translationX = 100;
  e.rotationZ = 200;
  const SpaceMouseMotion3d at1s = SpaceMouseMotionMapper::toMotion3d(e, 1.0);
  const SpaceMouseMotion3d at2s = SpaceMouseMotionMapper::toMotion3d(e, 2.0);
  EXPECT_DOUBLE_EQ(2 * at1s.panDelta.x(), at2s.panDelta.x());
  EXPECT_DOUBLE_EQ(2 * at1s.rotateZDeg, at2s.rotateZDeg);
}

TEST(SpaceMouseMotionMapperTest, test3dZoomMatches2dZoom) {
  SpaceMouseMotionEvent e;
  e.translationZ = -120;
  EXPECT_DOUBLE_EQ(SpaceMouseMotionMapper::toMotion2d(e, 0.5).zoomFactor,
                   SpaceMouseMotionMapper::toMotion3d(e, 0.5).zoomFactor);
}

/*******************************************************************************
 *  Test Methods: SpaceMouseMotionMapper::applySettings()
 ******************************************************************************/

TEST(SpaceMouseMotionMapperTest, testSettingsDefaultsLeaveEventUnchanged) {
  const WorkspaceSettingsItem_SpaceMouse settings(nullptr);
  SpaceMouseMotionEvent e;
  e.translationX = 12;
  e.translationY = -34;
  e.translationZ = 56;
  e.rotationX = -78;
  e.rotationY = 90;
  e.rotationZ = -123;
  EXPECT_EQ(e, SpaceMouseMotionMapper::applySettings(e, settings));
}

TEST(SpaceMouseMotionMapperTest, testSettingsSensitivityAndInvertPerAxis) {
  using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;
  WorkspaceSettingsItem_SpaceMouse settings(nullptr);
  settings.set(Axis::TranslationX, {2.0, false});
  settings.set(Axis::RotationY, {0.5, true});

  SpaceMouseMotionEvent e;
  e.translationX = 100;
  e.translationY = 100;  // Untouched axis.
  e.rotationY = 100;
  const SpaceMouseMotionEvent out = SpaceMouseMotionMapper::applySettings(e, settings);
  EXPECT_EQ(200, out.translationX);
  EXPECT_EQ(100, out.translationY);
  EXPECT_EQ(-50, out.rotationY);
}

TEST(SpaceMouseMotionMapperTest, testSettingsResultIsClampedToInt16) {
  using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;
  WorkspaceSettingsItem_SpaceMouse settings(nullptr);
  settings.set(Axis::TranslationX, {1000.0, false});
  settings.set(Axis::TranslationY, {1000.0, true});

  SpaceMouseMotionEvent e;
  e.translationX = 30000;
  e.translationY = 30000;
  const SpaceMouseMotionEvent out = SpaceMouseMotionMapper::applySettings(e, settings);
  EXPECT_EQ(std::numeric_limits<qint16>::max(), out.translationX);
  EXPECT_EQ(std::numeric_limits<qint16>::min(), out.translationY);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
