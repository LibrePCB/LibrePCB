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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_DRAWPLANE_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_DRAWPLANE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Plane;
class Layer;
class NetSignal;

namespace editor {

class CmdBoardPlaneEdit;
class LayerComboBox;

/*******************************************************************************
 *  Class BoardEditorState_DrawPlane
 ******************************************************************************/

/**
 * @brief The "draw plane" state/tool of the board editor
 */
class BoardEditorState_DrawPlane final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_DrawPlane() = delete;
  BoardEditorState_DrawPlane(const BoardEditorState_DrawPlane& other) = delete;
  explicit BoardEditorState_DrawPlane(const Context& context) noexcept;
  virtual ~BoardEditorState_DrawPlane() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToBoard(int index) noexcept override;

  // Operator Overloadings
  BoardEditorState_DrawPlane& operator=(const BoardEditorState_DrawPlane& rhs) =
      delete;

private:  // Methods
  bool startAddPlane(const Point& pos) noexcept;
  bool addSegment(const Point& pos) noexcept;
  bool updateLastVertexPosition(const Point& pos) noexcept;
  void setNetSignal(NetSignal* netsignal) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void layerComboBoxLayerChanged(const Layer& layer) noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;
  bool mAutoNetSignal;
  NetSignal* mLastNetSignal;
  const Layer* mLastLayer;
  Point mLastVertexPos;

  // Information about the current text to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Plane* mCurrentPlane;
  std::unique_ptr<CmdBoardPlaneEdit> mCurrentPlaneEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
