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

#ifndef LIBREPCB_EDITOR_SLINTOPENGLVIEW_H
#define LIBREPCB_EDITOR_SLINTOPENGLVIEW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtOpenGL>

#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class OpenGlObject;

/*******************************************************************************
 *  Struct OpenGlProjection
 ******************************************************************************/

struct OpenGlProjection {
  qreal fov;
  QPointF center;
  QMatrix4x4 transform;

  OpenGlProjection() noexcept : fov(15), center(), transform() {}
  OpenGlProjection(const OpenGlProjection& other) noexcept
    : fov(other.fov), center(other.center), transform(other.transform) {}
  OpenGlProjection(qreal fov, const QPointF& center,
                   const QMatrix4x4& transform) noexcept
    : fov(fov), center(center), transform(transform) {}
  OpenGlProjection interpolated(const OpenGlProjection& delta,
                                qreal factor) const noexcept {
    return OpenGlProjection(fov + delta.fov * factor,
                            center + delta.center * factor,
                            transform + delta.transform * factor);
  }
  bool operator!=(const OpenGlProjection& rhs) const noexcept {
    return (fov != rhs.fov) || (center != rhs.center) ||
        (transform != rhs.transform);
  }
  OpenGlProjection operator-(const OpenGlProjection& rhs) const noexcept {
    return OpenGlProjection(fov - rhs.fov, center - rhs.center,
                            transform - rhs.transform);
  }
  OpenGlProjection& operator=(const OpenGlProjection& rhs) noexcept {
    fov = rhs.fov;
    center = rhs.center;
    transform = rhs.transform;
    return *this;
  }
};

/*******************************************************************************
 *  Class SlintOpenGlView
 ******************************************************************************/

/**
 * @brief The SlintOpenGlView class
 */
class SlintOpenGlView final : public QObject, protected QOpenGLFunctions {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit SlintOpenGlView(
      const OpenGlProjection& projection = OpenGlProjection(),
      QObject* parent = nullptr) noexcept;
  SlintOpenGlView(const SlintOpenGlView& other) = delete;
  virtual ~SlintOpenGlView() noexcept;

  // Getters
  bool isPanning() const noexcept;
  const QStringList& getOpenGlErrors() const noexcept { return mErrors; }
  const OpenGlProjection& getProjection() const noexcept { return mProjection; }
  static QColor getBackgroundColor() noexcept { return QColor(230, 242, 255); }

  // General Methods
  void addObject(std::shared_ptr<OpenGlObject> obj) noexcept;
  void removeObject(std::shared_ptr<OpenGlObject> obj) noexcept;
  void setObjects(const QVector<std::shared_ptr<OpenGlObject>>& objs) noexcept;
  slint::Image render(float width, float height) noexcept;
  bool pointerEvent(const QPointF& pos,
                    slint::private_api::PointerEvent e) noexcept;
  bool scrollEvent(const QPointF& pos,
                   slint::private_api::PointerScrollEvent e) noexcept;
  void zoomIn() noexcept;
  void zoomOut() noexcept;
  void zoomAll() noexcept;

  // Operator Overloadings
  SlintOpenGlView& operator=(const SlintOpenGlView& rhs) = delete;

signals:
  void stateChanged();
  void contentChanged();

private:  // Methods
  void initializeGl() noexcept;
  void zoom(const QPointF& center, qreal factor) noexcept;
  void smoothTo(const OpenGlProjection& projection) noexcept;
  bool applyOpenGlProjection(const OpenGlProjection& projection) noexcept;
  QPointF toNormalizedPos(const QPointF& pos) const noexcept;
  QPointF toModelPos(const QPointF& pos) const noexcept;

private:  // Data
  std::unique_ptr<QOffscreenSurface> mSurface;
  std::unique_ptr<QOpenGLContext> mContext;
  std::unique_ptr<QOpenGLShaderProgram> mProgram;
  std::unique_ptr<QOpenGLFramebufferObject> mFbo;
  QStringList mErrors;
  QSizeF mViewSize;

  // State
  OpenGlProjection mProjection;
  QPointF mMousePressPosition;
  QMatrix4x4 mMousePressTransform;
  QPointF mMousePressCenter;
  QSet<slint::private_api::PointerEventButton> mPressedMouseButtons;

  // Transform Animation
  OpenGlProjection mAnimationDataStart;
  OpenGlProjection mAnimationDataDelta;
  QScopedPointer<QVariantAnimation> mAnimation;

  // Content
  QVector<std::shared_ptr<OpenGlObject>> mObjects;

  // Static Variables
  static constexpr qreal sCameraPosZ = 5.0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
