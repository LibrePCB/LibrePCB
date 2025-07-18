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

#ifndef LIBREPCB_EDITOR_SLINTGRAPHICSVIEW_H
#define LIBREPCB_EDITOR_SLINTGRAPHICSVIEW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicsscene.h"

#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtGui>

#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsScene;
class IF_GraphicsViewEventHandler;

/*******************************************************************************
 *  Class SlintGraphicsView
 ******************************************************************************/

/**
 * @brief The SlintGraphicsView class
 */
class SlintGraphicsView final : public QObject {
  Q_OBJECT

  struct Projection {
    QPointF offset;
    qreal scale = 1;

    Projection interpolated(const Projection& delta,
                            qreal factor) const noexcept {
      return Projection{
          offset + delta.offset * factor,
          scale + delta.scale * factor,
      };
    }
    bool operator!=(const Projection& rhs) const noexcept {
      return (offset != rhs.offset) || (scale != rhs.scale);
    }
    Projection operator-(const Projection& rhs) const noexcept {
      return Projection{
          offset - rhs.offset,
          scale - rhs.scale,
      };
    }
  };

public:
  // Constructors / Destructor
  explicit SlintGraphicsView(const QRectF& defaultSceneRect,
                             QObject* parent = nullptr) noexcept;
  SlintGraphicsView(const SlintGraphicsView& other) = delete;
  virtual ~SlintGraphicsView() noexcept;

  // Getters
  bool isPanning() const noexcept { return mPanning; }
  QPainterPath calcPosWithTolerance(const Point& pos,
                                    qreal multiplier) const noexcept;
  Point mapToScenePos(const QPointF& pos) const noexcept;

  // General Methods
  void setEventHandler(IF_GraphicsViewEventHandler* obj) noexcept;
  slint::Image render(GraphicsScene& scene, float width, float height) noexcept;
  bool pointerEvent(const QPointF& pos,
                    slint::private_api::PointerEvent e) noexcept;
  bool scrollEvent(const QPointF& pos,
                   slint::private_api::PointerScrollEvent e) noexcept;
  bool keyEvent(const slint::private_api::KeyEvent& e) noexcept;
  void scrollLeft() noexcept;
  void scrollRight() noexcept;
  void scrollUp() noexcept;
  void scrollDown() noexcept;
  void zoomIn() noexcept;
  void zoomOut() noexcept;
  void zoomToSceneRect(const QRectF& r) noexcept;

  // Static Methods
  static QRectF defaultSymbolSceneRect() noexcept;
  static QRectF defaultFootprintSceneRect() noexcept;
  static QRectF defaultSchematicSceneRect() noexcept;
  static QRectF defaultBoardSceneRect() noexcept;

  // Operator Overloadings
  SlintGraphicsView& operator=(const SlintGraphicsView& rhs) = delete;

signals:
  void stateChanged();
  void transformChanged();

private:  // Methods
  void scroll(const QPointF& delta) noexcept;
  void zoom(const QPointF& center, qreal factor) noexcept;
  void smoothTo(const Projection& projection) noexcept;
  bool applyProjection(const Projection& projection) noexcept;
  QRectF validateSceneRect(const QRectF& r) const noexcept;

private:  // Data
  const QRectF mDefaultSceneRect;
  IF_GraphicsViewEventHandler* mEventHandler;
  Projection mProjection;
  QSizeF mViewSize;

  GraphicsSceneMouseEvent mMouseEvent;
  QDeadlineTimer mLeftMouseButtonDoubleClickTimer;

  bool mPanning = false;
  QPointF mPanningStartScreenPos;
  QPointF mPanningStartScenePos;

  Projection mAnimationDataStart;
  Projection mAnimationDataDelta;
  std::unique_ptr<QVariantAnimation> mAnimation;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
