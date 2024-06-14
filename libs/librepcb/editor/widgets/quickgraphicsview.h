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
#include <librepcb/core/project/board/board.h>

#include <QtCore>
#include <QtQuick>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class BoardEditor;

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
  Q_PROPERTY(QObject* board READ getBoard WRITE setBoard NOTIFY boardChanged)

  // Getters
  QObject* getBoard() const noexcept;

  // Setters
  void setBoard(QObject* board) noexcept;

  // General Methods
  void paint(QPainter* painter) noexcept override;

public slots:
  void zoomIn() noexcept;
  void zoomOut() noexcept;
  void zoomAll() noexcept;

signals:
  void boardChanged(QObject* board);

protected:  // Methods
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void wheelEvent(QWheelEvent* e) override;
  void smoothTo(const QMatrix4x4& transform) noexcept;
  QVector2D toScenePos(const QMatrix4x4& t,
                       const QPointF& widgetPos) const noexcept;

private:
  QMatrix4x4 mTransform;
  QMatrix4x4 mMousePressTransform;
  QVector2D mMousePressScenePos;

  // Transform Animation
  QMatrix4x4 mAnimationTransformStart;
  QMatrix4x4 mAnimationTransformDelta;
  QScopedPointer<QVariantAnimation> mAnimation;

  // Content
  BoardEditor* mBoard = nullptr;

  // Static Variables
  static constexpr qreal sZoomStepFactor = 1.3;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
