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

#ifndef LIBREPCB_EDITOR_BOARD3DTAB_H
#define LIBREPCB_EDITOR_BOARD3DTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BoardPlaneFragmentsBuilder;

namespace editor {

class GuiApplication;
class OpenGlSceneBuilder;
class OpenGlView;

/*******************************************************************************
 *  Class Board3dTab
 ******************************************************************************/

/**
 * @brief The Board3dTab class
 */
class Board3dTab final : public WindowTab {
  Q_OBJECT

public:
  static const constexpr int sInitialFov = 15;
  struct Projection {
    qreal fov = sInitialFov;
    QPointF center;
    QMatrix4x4 transform;

    Projection interpolated(const Projection& delta,
                            qreal factor) const noexcept {
      return Projection{
          fov + delta.fov * factor,
          center + delta.center * factor,
          transform + delta.transform * factor,
      };
    }
    bool operator!=(const Projection& rhs) const noexcept {
      return (fov != rhs.fov) || (center != rhs.center) ||
          (transform != rhs.transform);
    }
    Projection operator-(const Projection& rhs) const noexcept {
      return Projection{
          fov - rhs.fov,
          center - rhs.center,
          transform - rhs.transform,
      };
    }
  };

  // Constructors / Destructor
  Board3dTab() = delete;
  Board3dTab(const Board3dTab& other) = delete;
  explicit Board3dTab(GuiApplication& app, std::shared_ptr<ProjectEditor2> prj,
                      int boardIndex, QObject* parent = nullptr) noexcept;
  virtual ~Board3dTab() noexcept;

  // General Methods
  ui::TabData getBaseUiData() const noexcept override;
  const ui::Board3dTabData& getUiData() const noexcept { return mUiData; }
  void setUiData(const ui::Board3dTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  bool trigger(ui::Action a) noexcept override;
  slint::Image renderScene(float width, float height) noexcept override;
  bool processScenePointerEvent(
      const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept override;
  bool processSceneScrolled(
      float x, float y,
      slint::private_api::PointerScrollEvent e) noexcept override;
  void zoomFit(float width, float height) noexcept override;
  void zoomIn(float width, float height) noexcept override;
  void zoomOut(float width, float height) noexcept override;

  // Operator Overloadings
  Board3dTab& operator=(const Board3dTab& rhs) = delete;

private:
  bool zoom(const QPointF& center, qreal factor) noexcept;
  void smoothTo(const Projection& projection) noexcept;
  bool applyProjection(const Projection& projection) noexcept;
  void requestRepaint() noexcept;

  ui::Board3dTabData mUiData;

  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlaneBuilder;
  std::shared_ptr<OpenGlView> mOpenGlView;
  std::shared_ptr<OpenGlSceneBuilder> mOpenGlSceneBuilder;

  Projection mProjection;

  QPointF mMousePressPosition;
  QMatrix4x4 mMousePressTransform;
  QPointF mMousePressCenter;
  QSet<slint::private_api::PointerEventButton> buttons;

  Projection mAnimationDataStart;
  Projection mAnimationDataDelta;
  QScopedPointer<QVariantAnimation> mAnimation;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
