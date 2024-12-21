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

#ifndef LIBREPCB_WINDOWTAB_H
#define LIBREPCB_WINDOWTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BoardPlaneFragmentsBuilder;

namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;
class OpenGlSceneBuilder;
class OpenGlView;

namespace app {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class WindowTab
 ******************************************************************************/

/**
 * @brief The WindowTab class
 */
class WindowTab : public QObject {
  Q_OBJECT

public:
  static const constexpr int sInitialFov = 15;
  struct Projection {
    QPointF offset;  // 2D
    qreal scale = 0;  // 2D
    qreal fov = sInitialFov;  // 3D
    QPointF center;  // 3D
    QMatrix4x4 transform;  // 3D

    Projection interpolated(const Projection& delta,
                            qreal factor) const noexcept {
      return Projection{
          offset + delta.offset * factor,
          scale + delta.scale * factor,
          fov + delta.fov * factor,
          center + delta.center * factor,
          transform + delta.transform * factor,
      };
    }
    bool operator!=(const Projection& rhs) const noexcept {
      return (offset != rhs.offset) || (scale != rhs.scale) ||
          (fov != rhs.fov) || (center != rhs.center) ||
          (transform != rhs.transform);
    }
    Projection operator-(const Projection& rhs) const noexcept {
      return Projection{
          offset - rhs.offset, scale - rhs.scale,         fov - rhs.fov,
          center - rhs.center, transform - rhs.transform,
      };
    }
  };

  // Constructors / Destructor
  WindowTab() = delete;
  WindowTab(const WindowTab& other) = delete;
  explicit WindowTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                     ui::TabType type, int objIndex, const QString& title,
                     QObject* parent = nullptr) noexcept;
  virtual ~WindowTab() noexcept;

  // General Methods
  const ui::Tab& getUiData() const noexcept { return mUiData; }
  std::shared_ptr<ProjectEditor> getProject() noexcept { return mProject; }
  int getObjIndex() const noexcept { return mObjIndex; }
  void activate() noexcept;
  void deactivate() noexcept;
  slint::Image renderScene(float width, float height) noexcept;
  bool processScenePointerEvent(float x, float y, float width, float height,
                                slint::private_api::PointerEvent e) noexcept;
  bool processSceneScrolled(float x, float y, float width, float height,
                            slint::private_api::PointerScrollEvent e) noexcept;
  void zoomFit(float width, float height) noexcept;
  void zoomIn(float width, float height) noexcept;
  void zoomOut(float width, float height) noexcept;

  // Operator Overloadings
  WindowTab& operator=(const WindowTab& rhs) = delete;

signals:
  void cursorCoordinatesChanged(qreal x, qreal y);
  void requestRepaint();

private:
  bool zoom(const QPointF& center, const QSizeF& size, qreal factor) noexcept;
  void smoothTo(const Projection& projection) noexcept;
  bool applyProjection(const Projection& projection) noexcept;

  ui::Tab mUiData;
  std::shared_ptr<ProjectEditor> mProject;
  int mObjIndex = -1;
  Projection mProjection;

  std::unique_ptr<IF_GraphicsLayerProvider> mLayerProvider;
  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlaneBuilder;
  std::shared_ptr<GraphicsScene> mScene;
  std::shared_ptr<OpenGlView> mOpenGlView;
  std::shared_ptr<OpenGlSceneBuilder> mOpenGlSceneBuilder;

  bool panning = false;
  QPointF startScenePos;

  QPointF mousePressPosition;
  QMatrix4x4 mousePressTransform;
  QPointF mousePressCenter;
  QSet<slint::private_api::PointerEventButton> buttons;

  Projection mAnimationDataStart;
  Projection mAnimationDataDelta;
  QScopedPointer<QVariantAnimation> mAnimation;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
