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

#ifndef LIBREPCB_COMMON_CMDHOLEEDIT_H
#define LIBREPCB_COMMON_CMDHOLEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../fileio/cmd/cmdlistelementinsert.h"
#include "../../fileio/cmd/cmdlistelementremove.h"
#include "../../fileio/cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"
#include "../../units/length.h"
#include "../../units/point.h"
#include "../hole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;

/*******************************************************************************
 *  Class CmdHoleEdit
 ******************************************************************************/

/**
 * @brief The CmdHoleEdit class
 */
class CmdHoleEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdHoleEdit() = delete;
  CmdHoleEdit(const CmdHoleEdit& other) = delete;
  explicit CmdHoleEdit(Hole& hole) noexcept;
  ~CmdHoleEdit() noexcept;

  // Setters
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirror(Qt::Orientation orientation, const Point& center,
              bool immediate) noexcept;
  void setDiameter(const PositiveLength& diameter, bool immediate) noexcept;

  // Operator Overloadings
  CmdHoleEdit& operator=(const CmdHoleEdit& rhs) = delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  Hole& mHole;

  // General Attributes
  Point mOldPosition;
  Point mNewPosition;
  PositiveLength mOldDiameter;
  PositiveLength mNewDiameter;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdHoleInsert =
    CmdListElementInsert<Hole, HoleListNameProvider, Hole::Event>;
using CmdHoleRemove =
    CmdListElementRemove<Hole, HoleListNameProvider, Hole::Event>;
using CmdHolesSwap =
    CmdListElementsSwap<Hole, HoleListNameProvider, Hole::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
