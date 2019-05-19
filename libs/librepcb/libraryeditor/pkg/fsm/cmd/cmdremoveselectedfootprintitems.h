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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDREMOVESELECTEDFOOTPRINTITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDREMOVESELECTEDFOOTPRINTITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../packageeditorstate.h"

#include <librepcb/common/undocommandgroup.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Class CmdRemoveSelectedFootprintItems
 ******************************************************************************/

/**
 * @brief The CmdRemoveSelectedFootprintItems class
 */
class CmdRemoveSelectedFootprintItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdRemoveSelectedFootprintItems() = delete;
  CmdRemoveSelectedFootprintItems(
      const CmdRemoveSelectedFootprintItems& other) = delete;
  CmdRemoveSelectedFootprintItems(
      const PackageEditorState::Context& context) noexcept;
  ~CmdRemoveSelectedFootprintItems() noexcept;

  // Operator Overloadings
  CmdRemoveSelectedFootprintItems& operator       =(
      const CmdRemoveSelectedFootprintItems& rhs) = delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  const PackageEditorState::Context& mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDREMOVESELECTEDFOOTPRINTITEMS_H
