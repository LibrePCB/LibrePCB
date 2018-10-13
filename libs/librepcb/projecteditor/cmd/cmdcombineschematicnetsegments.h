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

#ifndef LIBREPCB_PROJECT_CMDCOMBINESCHEMATICNETSEGMENTS_H
#define LIBREPCB_PROJECT_CMDCOMBINESCHEMATICNETSEGMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class SI_NetSegment;
class SI_NetLineAnchor;

namespace editor {

/*******************************************************************************
 *  Class CmdCombineSchematicNetSegments
 ******************************************************************************/

/**
 * @brief This undo command combines two schematic netsegments together
 *
 * @note Both netsegments must have the same netsignal!
 */
class CmdCombineSchematicNetSegments final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdCombineSchematicNetSegments() = delete;
  CmdCombineSchematicNetSegments(const CmdCombineSchematicNetSegments& other) =
      delete;
  CmdCombineSchematicNetSegments(SI_NetSegment&    toBeRemoved,
                                 SI_NetLineAnchor& oldAnchor,
                                 SI_NetSegment&    result,
                                 SI_NetLineAnchor& newAnchor) noexcept;
  ~CmdCombineSchematicNetSegments() noexcept;

  // Operator Overloadings
  CmdCombineSchematicNetSegments& operator       =(
      const CmdCombineSchematicNetSegments& rhs) = delete;

private:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

private:  // Data
  SI_NetSegment&    mOldSegment;
  SI_NetSegment&    mNewSegment;
  SI_NetLineAnchor& mOldAnchor;
  SI_NetLineAnchor& mNewAnchor;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDCOMBINESCHEMATICNETSEGMENTS_H
