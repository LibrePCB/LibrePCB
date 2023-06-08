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

#ifndef LIBREPCB_EDITOR_CMDZONEEDIT_H
#define LIBREPCB_EDITOR_CMDZONEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"
#include "cmdlistelementinsert.h"
#include "cmdlistelementremove.h"
#include "cmdlistelementsswap.h"

#include <librepcb/core/geometry/zone.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdZoneEdit
 ******************************************************************************/

/**
 * @brief The CmdZoneEdit class
 */
class CmdZoneEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdZoneEdit() = delete;
  CmdZoneEdit(const CmdZoneEdit& other) = delete;
  explicit CmdZoneEdit(Zone& zone) noexcept;
  ~CmdZoneEdit() noexcept;

  // Setters
  void setLayers(Zone::Layers layers, bool immediate) noexcept;
  void setRules(Zone::Rules rules, bool immediate) noexcept;
  void setOutline(const Path& path, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirrorGeometry(Qt::Orientation orientation, const Point& center,
                      bool immediate) noexcept;
  void mirrorLayers(bool immediate) noexcept;

  // Operator Overloadings
  CmdZoneEdit& operator=(const CmdZoneEdit& rhs) = delete;

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
  Zone& mZone;

  // General Attributes
  Zone::Layers mOldLayers;
  Zone::Layers mNewLayers;
  Zone::Rules mOldRules;
  Zone::Rules mNewRules;
  Path mOldOutline;
  Path mNewOutline;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdZoneInsert =
    CmdListElementInsert<Zone, ZoneListNameProvider, Zone::Event>;
using CmdZoneRemove =
    CmdListElementRemove<Zone, ZoneListNameProvider, Zone::Event>;
using CmdZonesSwap =
    CmdListElementsSwap<Zone, ZoneListNameProvider, Zone::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
