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
#include <librepcb/editor/graphics/graphicsscene.h>
#include <librepcb/editor/graphics/slintgraphicsview.h>

#include <QtCore>
#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SlintGraphicsViewTest : public ::testing::Test {
public:
  // Returns the half-width of the tolerance ellipse calcPosWithTolerance()
  // draws around the scene origin, which is `5 / scale`. Used as a
  // black-box way to observe the (otherwise private) current zoom scale.
  static qreal toleranceHalfWidth(const SlintGraphicsView& view) noexcept {
    const QPainterPath path = view.calcPosWithTolerance(Point(), 1);
    return path.boundingRect().width() / 2;
  }
};

/*******************************************************************************
 *  Test Methods: applyContinuousMotion()
 ******************************************************************************/

// A pan-only call (zoomFactor == 1) at the default scale (1) should move the
// projection offset by exactly the requested delta, the same way scroll()
// (used by the mouse wheel / keyboard shortcuts) would.
TEST_F(SlintGraphicsViewTest, applyContinuousMotionPansAtDefaultScale) {
  SlintGraphicsView view(SlintGraphicsView::defaultSchematicSceneRect(),
                         SlintGraphicsView::defaultMargins());

  const QPointF before = view.mapToScenePosPx(QPointF(0, 0), 1);
  view.applyContinuousMotion(QPointF(20, -10), 1);
  const QPointF after = view.mapToScenePosPx(QPointF(0, 0), 1);

  EXPECT_EQ(QPointF(20, -10), after - before);
}

// A no-op call (no pan, no zoom) must not change the projection at all, i.e.
// it must not emit transformChanged() -- otherwise a polled input device
// sending idle/zero frames would cause continuous unnecessary repaints.
TEST_F(SlintGraphicsViewTest, applyContinuousMotionNoOpDoesNotEmitSignal) {
  SlintGraphicsView view(SlintGraphicsView::defaultSchematicSceneRect(),
                         SlintGraphicsView::defaultMargins());

  QSignalSpy spy(&view, &SlintGraphicsView::transformChanged);
  view.applyContinuousMotion(QPointF(0, 0), 1);

  EXPECT_EQ(0, spy.count());
}

// A combined pan+zoom call must result in exactly one transformChanged()
// signal (not two, e.g. one for the pan and one for the zoom part) since it
// is meant to be a single atomic update of the projection.
TEST_F(SlintGraphicsViewTest, applyContinuousMotionEmitsSignalOnce) {
  SlintGraphicsView view(SlintGraphicsView::defaultSchematicSceneRect(),
                         SlintGraphicsView::defaultMargins());

  QSignalSpy spy(&view, &SlintGraphicsView::transformChanged);
  view.applyContinuousMotion(QPointF(5, 5), 1.5);

  EXPECT_EQ(1, spy.count());
}

// A zoom-only call must scale the view by exactly the given factor.
TEST_F(SlintGraphicsViewTest, applyContinuousMotionZoomsByGivenFactor) {
  SlintGraphicsView view(SlintGraphicsView::defaultSchematicSceneRect(),
                         SlintGraphicsView::defaultMargins());

  const qreal widthBefore = toleranceHalfWidth(view);
  view.applyContinuousMotion(QPointF(0, 0), 2);
  const qreal widthAfter = toleranceHalfWidth(view);

  // Tolerance half-width is `5 / scale`, so doubling the scale halves it.
  EXPECT_NEAR(widthBefore / 2, widthAfter, 1e-9);
}

// Unlike zoom() (used for the mouse wheel, which anchors on the cursor
// position), applyContinuousMotion() has no on-screen cursor to anchor to,
// so a zoom-only call must keep the *center of the view* stationary on
// screen.
TEST_F(SlintGraphicsViewTest, applyContinuousMotionZoomsAroundViewCenter) {
  SlintGraphicsView view(SlintGraphicsView::defaultSchematicSceneRect(),
                         SlintGraphicsView::defaultMargins());
  GraphicsScene scene;

  // Render once with a concrete size so mViewSize (and thus the view center
  // used as the zoom anchor) is well-defined.
  view.render(scene, 200, 150);
  const QPointF center(100, 75);
  const QPointF scenePosBefore = view.mapToScenePosPx(center, 1);

  view.applyContinuousMotion(QPointF(0, 0), 1.7);

  const QPointF scenePosAfter = view.mapToScenePosPx(center, 1);
  EXPECT_NEAR(scenePosBefore.x(), scenePosAfter.x(), 1e-6);
  EXPECT_NEAR(scenePosBefore.y(), scenePosAfter.y(), 1e-6);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
