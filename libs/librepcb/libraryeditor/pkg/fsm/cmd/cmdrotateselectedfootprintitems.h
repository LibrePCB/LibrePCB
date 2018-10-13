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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDROTATESELECTEDFOOTPRINTITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDROTATESELECTEDFOOTPRINTITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../packageeditorstate.h"

#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/angle.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Class CmdRotateSelectedFootprintItems
 ******************************************************************************/

/**
 * @brief The CmdRotateSelectedFootprintItems class
 *
 * @author  ubruhin
 * @date    2017-05-28
 */
class CmdRotateSelectedFootprintItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdRotateSelectedFootprintItems() = delete;
  CmdRotateSelectedFootprintItems(
      const CmdRotateSelectedFootprintItems& other) = delete;
  CmdRotateSelectedFootprintItems(const PackageEditorState::Context& context,
                                  const Angle& angle) noexcept;
  ~CmdRotateSelectedFootprintItems() noexcept;

  // Operator Overloadings
  CmdRotateSelectedFootprintItems& operator       =(
      const CmdRotateSelectedFootprintItems& rhs) = delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  const PackageEditorState::Context& mContext;
  Angle                              mAngle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDROTATESELECTEDFOOTPRINTITEMS_H
