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

#ifndef LIBREPCB_EDITOR_CMDCHANGEBUSOFSCHEMATICBUSSEGMENT_H
#define LIBREPCB_EDITOR_CMDCHANGEBUSOFSCHEMATICBUSSEGMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Bus;
class SI_BusSegment;

namespace editor {

/*******************************************************************************
 *  Class CmdChangeBusOfSchematicBusSegment
 ******************************************************************************/

/**
 * @brief The CmdChangeBusOfSchematicBusSegment class
 */
class CmdChangeBusOfSchematicBusSegment final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdChangeBusOfSchematicBusSegment() = delete;
  CmdChangeBusOfSchematicBusSegment(
      const CmdChangeBusOfSchematicBusSegment& other) = delete;
  CmdChangeBusOfSchematicBusSegment(SI_BusSegment& seg, Bus& newBus) noexcept;
  ~CmdChangeBusOfSchematicBusSegment() noexcept;

private:
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  void changeBusOfSegment();

private:
  SI_BusSegment& mSegment;
  Bus& mNewBus;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
