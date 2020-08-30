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

#ifndef LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_DRAWPLANE_H
#define LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_DRAWPLANE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/common/graphics/graphicslayername.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayerComboBox;

namespace project {

class NetSignal;
class BI_Plane;
class CmdBoardPlaneEdit;

namespace editor {

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
  BoardEditorState_DrawPlane()                                        = delete;
  BoardEditorState_DrawPlane(const BoardEditorState_DrawPlane& other) = delete;
  explicit BoardEditorState_DrawPlane(const Context& context) noexcept;
  virtual ~BoardEditorState_DrawPlane() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToBoard(int index) noexcept override;

  // Operator Overloadings
  BoardEditorState_DrawPlane& operator=(const BoardEditorState_DrawPlane& rhs) =
      delete;

private:  // Methods
  bool startAddPlane(Board& board, const Point& pos) noexcept;
  bool addSegment(const Point& pos) noexcept;
  bool updateLastVertexPosition(const Point& pos) noexcept;
  void setNetSignal(NetSignal* netsignal) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void layerComboBoxLayerChanged(const GraphicsLayerName& layerName) noexcept;
  void makeSelectedLayerVisible() noexcept;

private:  // Data
  // State
  bool              mIsUndoCmdActive;
  NetSignal*        mLastNetSignal;
  GraphicsLayerName mLastLayerName;
  Point             mLastVertexPos;

  // Information about the current text to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Plane*                         mCurrentPlane;
  QScopedPointer<CmdBoardPlaneEdit> mCurrentPlaneEditCmd;

  // Widgets for the command toolbar
  QList<QAction*>                       mActionSeparators;
  QScopedPointer<QLabel>                mNetSignalLabel;
  QScopedPointer<QComboBox>             mNetSignalComboBox;
  QScopedPointer<QLabel>                mLayerLabel;
  QScopedPointer<GraphicsLayerComboBox> mLayerComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
