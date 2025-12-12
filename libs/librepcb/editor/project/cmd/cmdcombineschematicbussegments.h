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

#ifndef LIBREPCB_EDITOR_CMDCOMBINESCHEMATICBUSSEGMENTS_H
#define LIBREPCB_EDITOR_CMDCOMBINESCHEMATICBUSSEGMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SI_BusJunction;
class SI_BusSegment;

namespace editor {

/*******************************************************************************
 *  Class CmdCombineSchematicBusSegments
 ******************************************************************************/

/**
 * @brief This undo command combines two schematic netsegments together
 *
 * @note Both netsegments must have the same netsignal!
 */
class CmdCombineSchematicBusSegments final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdCombineSchematicBusSegments() = delete;
  CmdCombineSchematicBusSegments(const CmdCombineSchematicBusSegments& other) =
      delete;
  CmdCombineSchematicBusSegments(SI_BusSegment& toBeRemoved,
                                 SI_BusJunction& oldAnchor,
                                 SI_BusSegment& result,
                                 SI_BusJunction& newAnchor) noexcept;
  ~CmdCombineSchematicBusSegments() noexcept;

  // Operator Overloadings
  CmdCombineSchematicBusSegments& operator=(
      const CmdCombineSchematicBusSegments& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

private:  // Data
  SI_BusSegment& mOldSegment;
  SI_BusSegment& mNewSegment;
  SI_BusJunction& mOldAnchor;
  SI_BusJunction& mNewAnchor;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
