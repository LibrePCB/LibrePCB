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

#include <librepcb/core/graphics/graphicslayername.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circle;
class CircleGraphicsItem;

namespace editor {

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
  QSet<EditorWidgetBase::Feature> getAvailableFeatures() const
      noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  PackageEditorState_DrawCircle& operator=(
      const PackageEditorState_DrawCircle& rhs) = delete;

private:  // Methods
  bool startAddCircle(const Point& pos) noexcept;
  bool updateCircleDiameter(const Point& pos) noexcept;
  bool finishAddCircle(const Point& pos) noexcept;
  bool abortAddCircle() noexcept;

  void layerComboBoxValueChanged(const GraphicsLayerName& layerName) noexcept;
  void lineWidthEditValueChanged(const UnsignedLength& value) noexcept;
  void fillCheckBoxCheckedChanged(bool checked) noexcept;
  void grabAreaCheckBoxCheckedChanged(bool checked) noexcept;

private:  // Types / Data
  QScopedPointer<CmdCircleEdit> mEditCmd;
  std::shared_ptr<Circle> mCurrentCircle;
  std::shared_ptr<CircleGraphicsItem> mCurrentGraphicsItem;

  // parameter memory
  GraphicsLayerName mLastLayerName;
  UnsignedLength mLastLineWidth;
  bool mLastFill;
  bool mLastGrabArea;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
