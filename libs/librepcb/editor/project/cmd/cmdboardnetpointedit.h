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

#ifndef LIBREPCB_EDITOR_CMDBOARDNETPOINTEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDNETPOINTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_NetPoint;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardNetPointEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardNetPointEdit class
 */
class CmdBoardNetPointEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdBoardNetPointEdit(BI_NetPoint& point) noexcept;
  ~CmdBoardNetPointEdit() noexcept;

  // Setters
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  BI_NetPoint& mNetPoint;

  // General Attributes
  Point mOldPos;
  Point mNewPos;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
