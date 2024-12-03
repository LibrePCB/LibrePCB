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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGONBASE_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGONBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class Polygon;

namespace editor {

class CmdPolygonEdit;
class PolygonGraphicsItem;

/*******************************************************************************
 *  Class SymbolEditorState_DrawPolygonBase
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_DrawPolygonBase class
 */
class SymbolEditorState_DrawPolygonBase : public SymbolEditorState {
  Q_OBJECT

public:
  // Types
  enum class Mode { LINE, ARC, RECT, POLYGON };

  // Constructors / Destructor
  SymbolEditorState_DrawPolygonBase() = delete;
  SymbolEditorState_DrawPolygonBase(
      const SymbolEditorState_DrawPolygonBase& other) = delete;
  SymbolEditorState_DrawPolygonBase(const Context& context, Mode mode) noexcept;
  virtual ~SymbolEditorState_DrawPolygonBase() noexcept;

  // General Methods
  bool processKeyPressed(const QKeyEvent& e) noexcept override;
  bool processKeyReleased(const QKeyEvent& e) noexcept override;
  bool entry() noexcept override;
  bool exit() noexcept override;
  QSet<EditorWidgetBase::Feature> getAvailableFeatures()
      const noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  SymbolEditorState_DrawPolygonBase& operator=(
      const SymbolEditorState_DrawPolygonBase& rhs) = delete;

private:  // Methods
  bool start() noexcept;
  bool abort(bool showErrMsgBox = true) noexcept;
  bool addNextSegment() noexcept;
  void updateCursorPosition(Qt::KeyboardModifiers modifiers) noexcept;
  void updatePolygonPath() noexcept;
  void updateOverlayText() noexcept;
  void updateStatusBarMessage() noexcept;

  void layerComboBoxValueChanged(const Layer& layer) noexcept;
  void lineWidthEditValueChanged(const UnsignedLength& value) noexcept;
  void angleEditValueChanged(const Angle& value) noexcept;
  void fillCheckBoxCheckedChanged(bool checked) noexcept;
  void grabAreaCheckBoxCheckedChanged(bool checked) noexcept;

private:  // Types / Data
  Mode mMode;
  bool mIsUndoCmdActive;
  std::unique_ptr<CmdPolygonEdit> mEditCmd;
  std::shared_ptr<Polygon> mCurrentPolygon;
  std::shared_ptr<PolygonGraphicsItem> mCurrentGraphicsItem;
  Point mLastScenePos;
  Point mCursorPos;

  // Arc tool state
  Point mArcCenter;
  bool mArcInSecondState;

  // parameter memory
  const Layer* mLastLayer;
  UnsignedLength mLastLineWidth;
  Angle mLastAngle;
  bool mLastFill;
  bool mLastGrabArea;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
