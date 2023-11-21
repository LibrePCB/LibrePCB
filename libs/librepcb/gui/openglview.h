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

#ifndef LIBREPCB_GUI_OPENGLVIEW_H
#define LIBREPCB_GUI_OPENGLVIEW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtQuick>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Class OpenGlView
 ******************************************************************************/

/**
 * @brief OpenGL viewer for 2D scenes
 */
class OpenGlView : public QQuickFramebufferObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  OpenGlView() noexcept;
  OpenGlView(const OpenGlView& other) noexcept = delete;
  virtual ~OpenGlView() noexcept;

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

private:
  QMatrix4x4 mTransform;
  QMatrix4x4 mMousePressTransform;
  QVector2D mMousePressPosition;

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

}  // namespace gui
}  // namespace librepcb

#endif
