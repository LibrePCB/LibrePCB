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

#ifndef LIBREPCB_EDITOR_GRAPHICSSCENETAB_H
#define LIBREPCB_EDITOR_GRAPHICSSCENETAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphics/graphicsscene.h"
#include "windowtab.h"

#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GuiApplication;
class IF_GraphicsLayerProvider;
class ProjectEditor2;

/*******************************************************************************
 *  Class GraphicsSceneTab
 ******************************************************************************/

/**
 * @brief The GraphicsSceneTab class
 */
class GraphicsSceneTab : public WindowTab {
  Q_OBJECT

public:
  struct Projection {
    QPointF offset;
    qreal scale = 0;

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

  // Constructors / Destructor
  GraphicsSceneTab() = delete;
  GraphicsSceneTab(const GraphicsSceneTab& other) = delete;
  explicit GraphicsSceneTab(GuiApplication& app,
                            QObject* parent = nullptr) noexcept;
  virtual ~GraphicsSceneTab() noexcept;

  // General Methods
  virtual void triggerAsync(ui::Action a) noexcept override;
  virtual slint::Image renderScene(float width, float height) noexcept override;
  virtual bool processScenePointerEvent(
      const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept override;
  virtual bool processSceneScrolled(
      float x, float y,
      slint::private_api::PointerScrollEvent e) noexcept override;

  // Operator Overloadings
  GraphicsSceneTab& operator=(const GraphicsSceneTab& rhs) = delete;

protected:
  QPainterPath calcPosWithTolerance(const Point& pos,
                                    qreal multiplier) const noexcept;
  Point mapToScenePos(const QPointF& pos) const noexcept;
  virtual void requestRepaint() noexcept = 0;
  virtual const LengthUnit* getCurrentUnit() const noexcept = 0;

private:
  virtual bool zoom(const QPointF& center, qreal factor) noexcept;
  virtual void smoothTo(const Projection& projection) noexcept;
  virtual bool applyProjection(const Projection& projection) noexcept;

protected:
  std::unique_ptr<IF_GraphicsLayerProvider> mLayerProvider;
  std::shared_ptr<GraphicsScene> mScene;

  QSizeF mViewSize;
  Projection mProjection;

  GraphicsSceneMouseEvent mMouseEvent;
  bool mMouseEventIsDoubleClick = false;
  QPointF mLeftMouseButtonDownPos;
  QDeadlineTimer mLeftMouseButtonDoubleClickTimer;

  bool mPanning = false;
  QPointF mPanningStartScreenPos;
  QPointF mPanningStartScenePos;

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
