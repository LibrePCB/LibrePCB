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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWCIRCLE_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWCIRCLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <librepcb/core/geometry/circle.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class CircleGraphicsItem;
class CmdCircleEdit;

/*******************************************************************************
 *  Class PackageEditorState_DrawCircle
 ******************************************************************************/

/**
 * @brief The PackageEditorState_DrawCircle class
 */
class PackageEditorState_DrawCircle final : public PackageEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_DrawCircle() = delete;
  PackageEditorState_DrawCircle(const PackageEditorState_DrawCircle& other) =
      delete;
  explicit PackageEditorState_DrawCircle(Context& context) noexcept;
  ~PackageEditorState_DrawCircle() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Connection to UI
  QSet<const Layer*> getAvailableLayers() const noexcept;
  const Layer& getLayer() const noexcept {
    return mCurrentProperties.getLayer();
  }
  void setLayer(const Layer& layer) noexcept;
  const UnsignedLength& getLineWidth() const noexcept {
    return mCurrentProperties.getLineWidth();
  }
  void setLineWidth(const UnsignedLength& width) noexcept;
  bool getFilled() const noexcept { return mCurrentProperties.isFilled(); }
  void setFilled(bool filled) noexcept;
  bool getGrabArea() const noexcept { return mCurrentProperties.isGrabArea(); }
  void setGrabArea(bool grabArea) noexcept;

  // Operator Overloadings
  PackageEditorState_DrawCircle& operator=(
      const PackageEditorState_DrawCircle& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void lineWidthChanged(const UnsignedLength& width);
  void filledChanged(bool filled);
  void grabAreaChanged(bool grabArea);

private:  // Methods
  bool startAddCircle(const Point& pos) noexcept;
  bool updateCircleDiameter(const Point& pos) noexcept;
  bool finishAddCircle(const Point& pos) noexcept;
  bool abortAddCircle() noexcept;

private:
  Circle mCurrentProperties;
  QHash<const Layer*, UnsignedLength> mUsedLineWidths;

  std::unique_ptr<CmdCircleEdit> mCurrentEditCmd;
  std::shared_ptr<Circle> mCurrentCircle;
  std::shared_ptr<CircleGraphicsItem> mCurrentGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
