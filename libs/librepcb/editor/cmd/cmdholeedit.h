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

#ifndef LIBREPCB_EDITOR_CMDHOLEEDIT_H
#define LIBREPCB_EDITOR_CMDHOLEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"
#include "cmdlistelementinsert.h"
#include "cmdlistelementremove.h"
#include "cmdlistelementsswap.h"

#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;

namespace editor {

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
  void setPath(const NonEmptyPath& path, bool immediate) noexcept;
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

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  Hole& mHole;

  // General Attributes
  NonEmptyPath mOldPath;
  NonEmptyPath mNewPath;
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

}  // namespace editor
}  // namespace librepcb

#endif
