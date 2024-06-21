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

#ifndef LIBREPCB_EDITOR_QUICKGRAPHICSVIEW_H
#define LIBREPCB_EDITOR_QUICKGRAPHICSVIEW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../graphics/graphicsscene.h"

#include <librepcb/core/types/point.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtQuick>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsScene;
class IF_GraphicsViewEventHandler;

/*******************************************************************************
 *  Class QuickGraphicsView
 ******************************************************************************/

/**
 * @brief OpenGL viewer for 2D scenes
 */
class QuickGraphicsView : public QQuickPaintedItem {
  Q_OBJECT

public:
  // Constructors / Destructor
  QuickGraphicsView() noexcept;
  QuickGraphicsView(const QuickGraphicsView& other) noexcept = delete;
  virtual ~QuickGraphicsView() noexcept;

  // Properties
  Q_PROPERTY(QObject* scene READ getScene WRITE setScene NOTIFY sceneChanged)

  // Getters
  QObject* getScene() const noexcept;
  Theme::GridStyle getGridStyle() const noexcept { return mGridStyle; }
  const PositiveLength& getGridInterval() const noexcept {
    return mGridInterval;
  }

  // Setters
  void setScene(QObject* scene) noexcept;
  void setBackgroundColors(const QColor& fill, const QColor& grid) noexcept;
  void setOverlayColors(const QColor& fill, const QColor& content) noexcept;
  void setInfoBoxColors(const QColor& fill, const QColor& text) noexcept;
  void setGridStyle(Theme::GridStyle style) noexcept;
  void setGridInterval(const PositiveLength& interval) noexcept;
  void setEventHandlerObject(
      IF_GraphicsViewEventHandler* eventHandler) noexcept;

  // General Methods
  QPointF mapFromSceneCoordinate(const QPointF& sceneCoordinate) const noexcept;
  QPointF mapToSceneCoordinate(const QPointF& widgetCoordinate) const noexcept;
  Point mapToScenePos(const QPoint& widgetCoordinate) const noexcept;
  Point mapGlobalPosToScenePos(const QPoint& globalPosPx, bool boundToView,
                               bool mapToGrid) const noexcept;
  QPainterPath calcPosWithTolerance(const Point& pos,
                                    qreal multiplier = 1) const noexcept;
  void paint(QPainter* painter) noexcept override;
  bool event(QEvent* event) noexcept override;

public slots:
  void zoomIn() noexcept;
  void zoomOut() noexcept;
  void zoomAll() noexcept;
  void showWaitingSpinner() noexcept;
  void hideWaitingSpinner() noexcept;

signals:
  void sceneChanged(QObject* scene);

  /**
   * @brief Cursor scene position changed signal
   *
   * @param pos   The new cursor position (*not* mapped to grid!)
   */
  void cursorScenePositionChanged(const Point& pos);

protected:  // Methods
  void mousePressEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;
  void mouseDoubleClickEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void hoverMoveEvent(QHoverEvent* e) override;
  void wheelEvent(QWheelEvent* e) override;
  void smoothTo(const QMatrix4x4& transform) noexcept;
  QVector2D toScenePos(const QMatrix4x4& t,
                       const QPointF& widgetPos) const noexcept;

private:
  template <typename T>
  void mouseMoveEventHandler(T* e) noexcept;
  void graphicsSceneChanged(const QList<QRectF>& region) noexcept;

private:
  // General Attributes
  // QScopedPointer<WaitingSpinnerWidget> mWaitingSpinnerWidget;
  // QScopedPointer<QLabel> mInfoBoxLabel;
  IF_GraphicsViewEventHandler* mEventHandlerObject;
  QPointer<GraphicsScene> mScene;
  // QVariantAnimation* mZoomAnimation;
  Theme::GridStyle mGridStyle;
  PositiveLength mGridInterval;
  QColor mBackgroundColor;
  QColor mGridColor;
  QColor mOverlayFillColor;
  QColor mOverlayContentColor;
  // QRectF mSceneRectMarker;
  // bool mOriginCrossVisible;
  // bool mGrayOut;

  /// If not nullopt, a cursor will be shown at the given position
  // tl::optional<std::pair<Point, CursorOptions>> mSceneCursor;

  // Configuration for the ruler overlay
  // struct RulerGauge {
  //  int xScale;
  //  LengthUnit unit;
  //  QString unitSeparator;
  //  Length minTickInterval;
  //  Length currentTickInterval;
  //};
  // QVector<RulerGauge> mRulerGauges;
  // tl::optional<std::pair<Point, Point>> mRulerPositions;

  // State
  QMatrix4x4 mTransform;
  QMatrix4x4 mMousePressTransform;
  QVector2D mMousePressScenePos;
  Qt::MouseButtons mPressedMouseButtons;
  volatile bool mPanningActive;
  Qt::MouseButton mPanningButton;
  QCursor mCursorBeforePanning;
  QGraphicsSceneMouseEvent mMouseMoveEvent;

  // Transform Animation
  QMatrix4x4 mAnimationTransformStart;
  QMatrix4x4 mAnimationTransformDelta;
  QScopedPointer<QVariantAnimation> mAnimation;

  // Static Variables
  static constexpr qreal sZoomStepFactor = 1.3;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
