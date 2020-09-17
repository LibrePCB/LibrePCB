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

#ifndef LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTADD_H
#define LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTADD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class NetSignal;
class Schematic;
class SI_NetSegment;

/*******************************************************************************
 *  Class CmdSchematicNetSegmentAdd
 ******************************************************************************/

/**
 * @brief The CmdSchematicNetSegmentAdd class
 */
class CmdSchematicNetSegmentAdd final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdSchematicNetSegmentAdd(SI_NetSegment& segment) noexcept;
  CmdSchematicNetSegmentAdd(Schematic& schematic,
                            NetSignal& netsignal) noexcept;
  ~CmdSchematicNetSegmentAdd() noexcept;

  // Getters
  SI_NetSegment* getNetSegment() const noexcept { return mNetSegment; }

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  Schematic& mSchematic;
  NetSignal& mNetSignal;
  SI_NetSegment* mNetSegment;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTADD_H
