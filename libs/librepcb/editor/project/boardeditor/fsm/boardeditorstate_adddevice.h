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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDDEVICE_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDDEVICE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class BI_Device;
class Board;
class ComponentInstance;

namespace editor {

class CmdDeviceInstanceEditAll;

/*******************************************************************************
 *  Class BoardEditorState_AddDevice
 ******************************************************************************/

/**
 * @brief The "add device" state/tool of the board editor
 */
class BoardEditorState_AddDevice final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_AddDevice() = delete;
  BoardEditorState_AddDevice(const BoardEditorState_AddDevice& other) = delete;
  BoardEditorState_AddDevice(const Context& context) noexcept;
  virtual ~BoardEditorState_AddDevice() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAddDevice(ComponentInstance& component,
                                const Uuid& device,
                                const Uuid& footprint) noexcept override;
  virtual bool processRotateCw() noexcept override;
  virtual bool processRotateCcw() noexcept override;
  virtual bool processFlipHorizontal() noexcept override;
  virtual bool processFlipVertical() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  BoardEditorState_AddDevice& operator=(const BoardEditorState_AddDevice& rhs) =
      delete;

private:
  // Private Methods
  bool addDevice(ComponentInstance& cmp, const Uuid& dev,
                 const Uuid& fpt) noexcept;
  bool rotateDevice(const Angle& angle) noexcept;
  bool mirrorDevice(Qt::Orientation orientation) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;

  // State
  bool mIsUndoCmdActive;

  // Information about the current device to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Device* mCurrentDeviceToPlace;
  QScopedPointer<CmdDeviceInstanceEditAll> mCurrentDeviceEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
