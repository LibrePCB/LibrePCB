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

#ifndef LIBREPCB_EDITOR_CMDFOOTPRINTEDIT_H
#define LIBREPCB_EDITOR_CMDFOOTPRINTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlistelementinsert.h"
#include "../../cmd/cmdlistelementremove.h"
#include "../../cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"

#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/types/elementname.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdFootprintEdit
 ******************************************************************************/

/**
 * @brief The CmdFootprintEdit class
 */
class CmdFootprintEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdFootprintEdit() = delete;
  CmdFootprintEdit(const CmdFootprintEdit& other) = delete;
  explicit CmdFootprintEdit(Footprint& fpt) noexcept;
  ~CmdFootprintEdit() noexcept;

  // Setters
  void setName(const ElementName& name) noexcept;
  void setModelPosition(const Point3D& pos) noexcept;
  void setModelRotation(const Angle3D& rot) noexcept;
  void setModels(const QSet<Uuid>& models) noexcept;

  // Operator Overloadings
  CmdFootprintEdit& operator=(const CmdFootprintEdit& rhs) = delete;

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
  Footprint& mFootprint;

  // General Attributes
  ElementName mOldName;
  ElementName mNewName;
  Point3D mOldModelPosition;
  Point3D mNewModelPosition;
  Angle3D mOldModelRotation;
  Angle3D mNewModelRotation;
  QSet<Uuid> mOldModels;
  QSet<Uuid> mNewModels;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdFootprintInsert =
    CmdListElementInsert<Footprint, FootprintListNameProvider,
                         Footprint::Event>;
using CmdFootprintRemove =
    CmdListElementRemove<Footprint, FootprintListNameProvider,
                         Footprint::Event>;
using CmdFootprintsSwap =
    CmdListElementsSwap<Footprint, FootprintListNameProvider, Footprint::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
