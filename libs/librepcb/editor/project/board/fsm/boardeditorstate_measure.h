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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_MEASURE_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_MEASURE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class MeasureTool;

/*******************************************************************************
 *  Class BoardEditorState_Measure
 ******************************************************************************/

/**
 * @brief The "measure" state/tool of the board editor
 */
class BoardEditorState_Measure final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_Measure() = delete;
  BoardEditorState_Measure(const BoardEditorState_Measure& other) = delete;
  explicit BoardEditorState_Measure(const Context& context) noexcept;
  virtual ~BoardEditorState_Measure() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processCopy() noexcept override;
  virtual bool processRemove() noexcept override;
  virtual bool processAbortCommand() noexcept override;
  virtual bool processKeyPressed(
      const GraphicsSceneKeyEvent& e) noexcept override;
  virtual bool processKeyReleased(
      const GraphicsSceneKeyEvent& e) noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToBoard(int index) noexcept override;
  virtual void processSwitchedBoard() noexcept override;

  // Operator Overloadings
  BoardEditorState_Measure& operator=(const BoardEditorState_Measure& rhs) =
      delete;

private:  // Data
  QScopedPointer<MeasureTool> mTool;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
