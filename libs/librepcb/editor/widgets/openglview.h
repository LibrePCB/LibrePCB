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

#ifndef LIBREPCB_EDITOR_OPENGLVIEW_H
#define LIBREPCB_EDITOR_OPENGLVIEW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtOpenGL>
#include <QtOpenGLWidgets>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class OpenGlObject;
class WaitingSpinnerWidget;

/*******************************************************************************
 *  Class OpenGlView
 ******************************************************************************/

/**
 * @brief OpenGL 3D viewer widget
 */
class OpenGlView final : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit OpenGlView(QWidget* parent = nullptr) noexcept;
  OpenGlView(const OpenGlView& other) = delete;
  virtual ~OpenGlView() noexcept;

  // Getters
  qint64 getIdleTimeMs() const noexcept { return mIdleTimeMs; }

  // General Methods
  void addObject(std::shared_ptr<OpenGlObject> obj) noexcept;
  void removeObject(std::shared_ptr<OpenGlObject> obj) noexcept;
  void setObjects(const QVector<std::shared_ptr<OpenGlObject>>& objs) noexcept;
  void zoomIn() noexcept;
  void zoomOut() noexcept;
  void zoomAll() noexcept;
  void startSpinning() noexcept;
  void stopSpinning(QStringList errors) noexcept;

  // Operator Overloadings
  OpenGlView& operator=(const OpenGlView& rhs) = delete;

protected:
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void wheelEvent(QWheelEvent* e) override;
  void smoothTo(qreal fov, const QPointF& center,
                const QMatrix4x4& transform) noexcept;
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

private:
  void zoom(const QPointF& center, qreal factor) noexcept;
  QPointF toNormalizedPos(const QPointF& pos) const noexcept;
  QPointF toModelPos(const QPointF& pos) const noexcept;

private:
  QScopedPointer<QVBoxLayout> mLayout;
  QScopedPointer<QLabel> mErrorLabel;
  bool mInitialized;
  QOpenGLShaderProgram mProgram;
  qreal mProjectionAspectRatio;
  qreal mProjectionFov;
  QPointF mProjectionCenter;
  QMatrix4x4 mTransform;
  QPoint mMousePressPosition;
  QMatrix4x4 mMousePressTransform;
  QPointF mMousePressCenter;
  qint64 mIdleTimeMs;
  QScopedPointer<WaitingSpinnerWidget> mWaitingSpinner;

  // Transform Animation
  struct TransformData {
    qreal fov;
    QPointF center;
    QMatrix4x4 transform;
  };
  TransformData mAnimationDataStart;
  TransformData mAnimationDataDelta;
  QScopedPointer<QVariantAnimation> mAnimation;

  // Content
  QVector<std::shared_ptr<OpenGlObject>> mObjects;

  // Static Variables
  static constexpr qreal sCameraPosZ = 5.0;
  static constexpr qreal sInitialFov = 15.0;
  static constexpr qreal sZoomStepFactor = 1.3;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
