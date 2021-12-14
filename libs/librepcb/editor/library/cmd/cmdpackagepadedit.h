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

#ifndef LIBREPCB_EDITOR_CMDPACKAGEPADEDIT_H
#define LIBREPCB_EDITOR_CMDPACKAGEPADEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlistelementinsert.h"
#include "../../cmd/cmdlistelementremove.h"
#include "../../cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"

#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdPackagePadEdit
 ******************************************************************************/

/**
 * @brief The CmdPackagePadEdit class
 */
class CmdPackagePadEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdPackagePadEdit() = delete;
  CmdPackagePadEdit(const CmdPackagePadEdit& other) = delete;
  explicit CmdPackagePadEdit(PackagePad& pad) noexcept;
  ~CmdPackagePadEdit() noexcept;

  // Setters
  void setName(const CircuitIdentifier& name) noexcept;

  // Operator Overloadings
  CmdPackagePadEdit& operator=(const CmdPackagePadEdit& rhs) = delete;

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
  PackagePad& mPad;

  // General Attributes
  CircuitIdentifier mOldName;
  CircuitIdentifier mNewName;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdPackagePadInsert =
    CmdListElementInsert<PackagePad, PackagePadListNameProvider,
                         PackagePad::Event>;
using CmdPackagePadRemove =
    CmdListElementRemove<PackagePad, PackagePadListNameProvider,
                         PackagePad::Event>;
using CmdPackagePadsSwap =
    CmdListElementsSwap<PackagePad, PackagePadListNameProvider,
                        PackagePad::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
