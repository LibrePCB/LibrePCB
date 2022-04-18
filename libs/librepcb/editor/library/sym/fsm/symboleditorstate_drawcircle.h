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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWCIRCLE_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWCIRCLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

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
 *  Class SymbolEditorState_DrawCircle
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_DrawCircle class
 */
class SymbolEditorState_DrawCircle final : public SymbolEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_DrawCircle() = delete;
  SymbolEditorState_DrawCircle(const SymbolEditorState_DrawCircle& other) =
      delete;
  explicit SymbolEditorState_DrawCircle(const Context& context) noexcept;
  ~SymbolEditorState_DrawCircle() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  SymbolEditorState_DrawCircle& operator=(
      const SymbolEditorState_DrawCircle& rhs) = delete;

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
