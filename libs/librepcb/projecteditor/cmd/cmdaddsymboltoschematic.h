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

#ifndef LIBREPCB_PROJECT_CMDADDSYMBOLTOSCHEMATIC_H
#define LIBREPCB_PROJECT_CMDADDSYMBOLTOSCHEMATIC_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace workspace {
class Workspace;
}

namespace library {
class Symbol;
}

namespace project {

class Schematic;
class ComponentInstance;
class SI_Symbol;

namespace editor {

/*******************************************************************************
 *  Class CmdAddSymbolToSchematic
 ******************************************************************************/

/**
 * @brief The CmdAddSymbolToSchematic class
 */
class CmdAddSymbolToSchematic final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdAddSymbolToSchematic(workspace::Workspace& workspace, Schematic& schematic,
                          ComponentInstance& cmpInstance,
                          const Uuid&        symbolItem,
                          const Point&       position = Point(),
                          const Angle&       angle    = Angle()) noexcept;
  ~CmdAddSymbolToSchematic() noexcept;

  // Getters
  SI_Symbol* getSymbolInstance() const noexcept { return mSymbolInstance; }

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables

  // Attributes from the constructor
  workspace::Workspace& mWorkspace;
  Schematic&            mSchematic;
  ComponentInstance&    mComponentInstance;
  Uuid                  mSymbolItemUuid;
  Point                 mPosition;
  Angle                 mAngle;

  SI_Symbol* mSymbolInstance;  // the created symbol instance
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDADDSYMBOLTOSCHEMATIC_H
