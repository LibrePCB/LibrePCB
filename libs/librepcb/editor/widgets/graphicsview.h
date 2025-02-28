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

#ifndef LIBREPCB_EDITOR_GRAPHICSVIEW_H
#define LIBREPCB_EDITOR_GRAPHICSVIEW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsScene;
class IF_GraphicsViewEventHandler;
class WaitingSpinnerWidget;

/*******************************************************************************
 *  Class GraphicsView
 ******************************************************************************/

/**
 * @brief The GraphicsView class
 */
class GraphicsView final : public QGraphicsView {
  Q_OBJECT

public:
  // Constructors / Destructor
  GraphicsView(const GraphicsView& other) = delete;
  explicit GraphicsView(
      QWidget* parent = nullptr,
      IF_GraphicsViewEventHandler* eventHandler = nullptr) noexcept;
  ~GraphicsView() noexcept;

  // Getters
  GraphicsScene* getScene() const noexcept { return mScene; }
  QRectF getVisibleSceneRect() const noexcept;
  bool getUseOpenGl() const noexcept { return mUseOpenGl; }
  bool isMouseButtonPressed(Qt::MouseButtons btn) const noexcept {
    return mPressedMouseButtons & btn;
  }
  qint64 getIdleTimeMs() const noexcept { return mIdleTimeMs; }

  // Setters
  void setSpinnerColor(const QColor& color) noexcept;
  void setInfoBoxColors(const QColor& fill, const QColor& text) noexcept;
  void setUseOpenGl(bool useOpenGl) noexcept;
  void setScene(GraphicsScene* scene) noexcept;
  void setVisibleSceneRect(const QRectF& rect) noexcept;
  void setInfoBoxText(const QString& text) noexcept;
  void setEventHandlerObject(
      IF_GraphicsViewEventHandler* eventHandler) noexcept;

  // General Methods
  Point mapGlobalPosToScenePos(const QPoint& globalPosPx) const noexcept;
  QPainterPath calcPosWithTolerance(const Point& pos,
                                    qreal multiplier = 1) const noexcept;
  void handleMouseWheelEvent(QGraphicsSceneWheelEvent* event) noexcept;

  // Operator Overloadings
  GraphicsView& operator=(const GraphicsView& rhs) = delete;

public slots:

  // Public Slots
  void zoomIn() noexcept;
  void zoomOut() noexcept;
  void zoomAll() noexcept;
  void zoomToRect(const QRectF& rect) noexcept;
  void showWaitingSpinner() noexcept;
  void hideWaitingSpinner() noexcept;

signals:
  /**
   * @brief Cursor scene position changed signal
   *
   * @param pos   The new cursor position (*not* mapped to grid!)
   */
  void cursorScenePositionChanged(const Point& pos);

private slots:

  // Private Slots
  void zoomAnimationValueChanged(const QVariant& value) noexcept;

private:
  // Inherited Methods
  void wheelEvent(QWheelEvent* event);
  bool eventFilter(QObject* obj, QEvent* event);

  // General Attributes
  QScopedPointer<WaitingSpinnerWidget> mWaitingSpinnerWidget;
  QScopedPointer<QLabel> mInfoBoxLabel;
  IF_GraphicsViewEventHandler* mEventHandlerObject;
  GraphicsScene* mScene;
  QVariantAnimation* mZoomAnimation;
  bool mUseOpenGl;

  // State
  volatile bool mPanningActive;
  Qt::MouseButton mPanningButton;
  Qt::MouseButtons mPressedMouseButtons;
  QCursor mCursorBeforePanning;
  qint64 mIdleTimeMs;

  // Static Variables
  static constexpr qreal sZoomStepFactor = 1.3;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
