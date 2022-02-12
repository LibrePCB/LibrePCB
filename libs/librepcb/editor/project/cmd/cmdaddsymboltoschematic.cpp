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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdaddsymboltoschematic.h"

#include "../../project/cmd/cmdprojectlibraryaddelement.h"
#include "../../project/cmd/cmdsymbolinstanceadd.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdAddSymbolToSchematic::CmdAddSymbolToSchematic(
    Workspace& workspace, Schematic& schematic, ComponentInstance& cmpInstance,
    const Uuid& symbolItem, const Point& position, const Angle& angle) noexcept
  : UndoCommandGroup(tr("Add symbol")),
    mWorkspace(workspace),
    mSchematic(schematic),
    mComponentInstance(cmpInstance),
    mSymbolItemUuid(symbolItem),
    mPosition(position),
    mAngle(angle),
    mSymbolInstance(nullptr) {
}

CmdAddSymbolToSchematic::~CmdAddSymbolToSchematic() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdAddSymbolToSchematic::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // get the symbol UUID
  const ComponentSymbolVariantItem& item =
      *mComponentInstance.getSymbolVariant().getSymbolItems().get(
          mSymbolItemUuid);  // can throw
  Uuid symbolUuid = item.getSymbolUuid();

  // if there is no such symbol in the project's library, copy it from the
  // workspace library to the project's library
  if (!mSchematic.getProject().getLibrary().getSymbol(symbolUuid)) {
    FilePath symFp = mWorkspace.getLibraryDb().getLatest<Symbol>(symbolUuid);
    if (!symFp.isValid()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The symbol with the UUID \"%1\" does not exist in the "
             "workspace library!")
              .arg(symbolUuid.toStr()));
    }
    Symbol* sym = new Symbol(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(TransactionalFileSystem::openRO(symFp))));
    CmdProjectLibraryAddElement<Symbol>* cmdAddToLibrary =
        new CmdProjectLibraryAddElement<Symbol>(
            mSchematic.getProject().getLibrary(), *sym);
    execNewChildCmd(cmdAddToLibrary);  // can throw
  }

  // create the new symbol (schematic takes ownership)
  mSymbolInstance =
      new SI_Symbol(mSchematic, mComponentInstance, mSymbolItemUuid, mPosition,
                    mAngle);  // can throw

  // add a new symbol instance to the schematic
  execNewChildCmd(new CmdSymbolInstanceAdd(*mSymbolInstance));  // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
