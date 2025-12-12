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

#ifndef LIBREPCB_EDITOR_CMDSCHEMATICBUSSEGMENTADDELEMENTS_H
#define LIBREPCB_EDITOR_CMDSCHEMATICBUSSEGMENTADDELEMENTS_H

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

class SI_BusJunction;
class SI_BusLine;
class SI_BusSegment;

namespace editor {

/*******************************************************************************
 *  Class CmdSchematicBusSegmentAddElements
 ******************************************************************************/

/**
 * @brief The CmdSchematicBusSegmentAddElements class
 */
class CmdSchematicBusSegmentAddElements final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdSchematicBusSegmentAddElements(SI_BusSegment& segment) noexcept;
  ~CmdSchematicBusSegmentAddElements() noexcept;

  // General Methods
  SI_BusJunction* addJunction(SI_BusJunction& junction);
  SI_BusJunction* addJunction(const Point& position);
  SI_BusLine* addLine(SI_BusLine& line);
  SI_BusLine* addLine(SI_BusJunction& a, SI_BusJunction& b);

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  SI_BusSegment& mSegment;
  QList<SI_BusJunction*> mJunctions;
  QList<SI_BusLine*> mLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
