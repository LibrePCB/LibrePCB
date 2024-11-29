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

#ifndef LIBREPCB_WINDOWSECTION_H
#define LIBREPCB_WINDOWSECTION_H

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
 *  Class WindowSection
 ******************************************************************************/

/**
 * @brief The WindowSection class
 */
class WindowSection : public QObject {
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
  struct Tab {
    std::shared_ptr<ProjectEditor> project;
    ui::TabType type;
    int objIndex = -1;
    Projection projection;
  };

  // Constructors / Destructor
  WindowSection() = delete;
  WindowSection(const WindowSection& other) = delete;
  explicit WindowSection(GuiApplication& app, int index,
                         QObject* parent = nullptr) noexcept;
  virtual ~WindowSection() noexcept;

  // General Methods
  void setIndex(int index) noexcept;
  int getIndex() const noexcept { return mUiData.index; }
  const ui::WindowSection& getUiData() const noexcept { return mUiData; }
  int getTabCount() const noexcept { return tabs.count(); }
  Tab* getTab(int index) noexcept {
    return ((index >= 0) && (index < tabs.count())) ? &tabs[index] : nullptr;
  }
  std::shared_ptr<ProjectEditor> getCurrentProject() noexcept;
  void addTab(std::shared_ptr<ProjectEditor> prj, ui::TabType type,
              int objIndex, const QString& title) noexcept;
  void closeTab(int index) noexcept;
  void setCurrentTab(int index) noexcept;
  slint::Image renderScene(int tab, float width, float height) noexcept;
  bool processScenePointerEvent(float x, float y, float width, float height,
                                slint::private_api::PointerEvent e) noexcept;
  bool processSceneScrolled(float x, float y, float width, float height,
                            slint::private_api::PointerScrollEvent e) noexcept;
  void zoomFit(float width, float height) noexcept;
  void zoomIn(float width, float height) noexcept;
  void zoomOut(float width, float height) noexcept;

  // Operator Overloadings
  WindowSection& operator=(const WindowSection& rhs) = delete;

signals:
  void uiDataChanged(int section);
  void currentProjectChanged(std::shared_ptr<ProjectEditor> prj);
  void cursorCoordinatesChanged(qreal x, qreal y);

private:
  bool zoom(const QPointF& center, const QSizeF& size, qreal factor) noexcept;
  void smoothTo(Tab& tab, const Projection& projection) noexcept;
  bool applyProjection(Tab& tab, const Projection& projection) noexcept;

  ui::WindowSection mUiData;
  std::unique_ptr<IF_GraphicsLayerProvider> mLayerProvider;
  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlaneBuilder;

  QList<Tab> tabs;
  std::shared_ptr<GraphicsScene> scene;
  std::shared_ptr<OpenGlView> openGlView;
  std::shared_ptr<OpenGlSceneBuilder> openGlSceneBuilder;
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
