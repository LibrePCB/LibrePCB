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

#ifndef LIBREPCB_CMDHOLEEDIT_H
#define LIBREPCB_CMDHOLEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"
#include "../../units/all_length_units.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Hole;

/*******************************************************************************
 *  Class CmdHoleEdit
 ******************************************************************************/

/**
 * @brief The CmdHoleEdit class
 */
class CmdHoleEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdHoleEdit()                         = delete;
  CmdHoleEdit(const CmdHoleEdit& other) = delete;
  explicit CmdHoleEdit(Hole& hole) noexcept;
  ~CmdHoleEdit() noexcept;

  // Setters
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
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
  Point          mOldPosition;
  Point          mNewPosition;
  PositiveLength mOldDiameter;
  PositiveLength mNewDiameter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_CMDHOLEEDIT_H
