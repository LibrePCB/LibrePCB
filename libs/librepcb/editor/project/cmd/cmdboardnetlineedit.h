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

#ifndef LIBREPCB_EDITOR_CMDBOARDNETLINEEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDNETLINEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/project/board/items/bi_netline.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdBoardNetLineEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardNetLineEdit class
 */
class CmdBoardNetLineEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdBoardNetLineEdit(BI_NetLine& netline) noexcept;
  ~CmdBoardNetLineEdit() noexcept;

  // Setters
  void setLayer(GraphicsLayer& layer) noexcept;
  void setWidth(const PositiveLength& width) noexcept;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  BI_NetLine& mNetLine;
  GraphicsLayer* mOldLayer;
  GraphicsLayer* mNewLayer;
  PositiveLength mOldWidth;
  PositiveLength mNewWidth;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
