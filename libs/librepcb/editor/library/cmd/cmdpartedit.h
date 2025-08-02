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

#ifndef LIBREPCB_EDITOR_CMDPARTEDIT_H
#define LIBREPCB_EDITOR_CMDPARTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlistelementinsert.h"
#include "../../cmd/cmdlistelementremove.h"
#include "../../cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"

#include <librepcb/core/library/dev/part.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdPartEdit
 ******************************************************************************/

/**
 * @brief The CmdPartEdit class
 */
class CmdPartEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdPartEdit() = delete;
  CmdPartEdit(const CmdPartEdit& other) = delete;
  explicit CmdPartEdit(const std::shared_ptr<Part>& part) noexcept;
  ~CmdPartEdit() noexcept;

  // Setters
  void setMpn(const SimpleString& value) noexcept;
  void setManufacturer(const SimpleString& value) noexcept;

  // Operator Overloadings
  CmdPartEdit& operator=(const CmdPartEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  std::shared_ptr<Part> mPart;

  SimpleString mOldMpn;
  SimpleString mNewMpn;
  SimpleString mOldManufacturer;
  SimpleString mNewManufacturer;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdPartInsert =
    CmdListElementInsert<Part, PartListNameProvider, Part::Event>;
using CmdPartRemove =
    CmdListElementRemove<Part, PartListNameProvider, Part::Event>;
using CmdPartsSwap =
    CmdListElementsSwap<Part, PartListNameProvider, Part::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
