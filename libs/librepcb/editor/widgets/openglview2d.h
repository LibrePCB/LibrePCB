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

#ifndef LIBREPCB_EDITOR_OPENGLVIEW2D_H
#define LIBREPCB_EDITOR_OPENGLVIEW2D_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtQuick>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class OpenGlView2D
 ******************************************************************************/

/**
 * @brief OpenGL viewer for 2D scenes
 */
class OpenGlView2D : public QQuickFramebufferObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  OpenGlView2D() noexcept;
  OpenGlView2D(const OpenGlView2D& other) noexcept = delete;
  virtual ~OpenGlView2D() noexcept;

  // Getters
  const QMatrix4x4& getTransform() const noexcept { return mTransform; }

  // General Methods
  QQuickFramebufferObject::Renderer* createRenderer() const noexcept override;

public slots:
  void zoomIn() noexcept;
  void zoomOut() noexcept;
  void zoomAll() noexcept;

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

  // Static Variables
  static constexpr qreal sZoomStepFactor = 1.3;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
