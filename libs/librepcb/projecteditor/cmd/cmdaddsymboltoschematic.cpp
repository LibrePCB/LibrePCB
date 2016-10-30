/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdaddsymboltoschematic.h"
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/project/project.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/library/cmd/cmdprojectlibraryaddelement.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceadd.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdAddSymbolToSchematic::CmdAddSymbolToSchematic(workspace::Workspace& workspace,
        Schematic& schematic, ComponentInstance& cmpInstance, const Uuid& symbolItem,
        const Point& position, const Angle& angle) noexcept :
    UndoCommandGroup(tr("Add symbol")),
    mWorkspace(workspace), mSchematic(schematic), mComponentInstance(cmpInstance),
    mSymbolItemUuid(symbolItem), mPosition(position), mAngle(angle),
    mCmdAddToSchematic(nullptr)
{
}

CmdAddSymbolToSchematic::~CmdAddSymbolToSchematic() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

SI_Symbol* CmdAddSymbolToSchematic::getSymbolInstance() const noexcept
{
    Q_ASSERT(mCmdAddToSchematic);
    return mCmdAddToSchematic ? mCmdAddToSchematic->getSymbolInstance() : nullptr;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdAddSymbolToSchematic::performExecute()
{
    // get the symbol UUID
    const library::ComponentSymbolVariantItem& item = *mComponentInstance.getSymbolVariant().getSymbolItems().get(mSymbolItemUuid); // can throw
    Uuid symbolUuid = item.getSymbolUuid();

    // if there is no such symbol in the project's library, copy it from the
    // workspace library to the project's library
    if (!mSchematic.getProject().getLibrary().getSymbol(symbolUuid)) {
        FilePath symFp = mWorkspace.getLibraryDb().getLatestSymbol(symbolUuid);
        if (!symFp.isValid()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The symbol with the UUID \"%1\" does not exist in the "
                "workspace library!")).arg(symbolUuid.toStr()));
        }
        library::Symbol* sym = new library::Symbol(symFp, true);
        CmdProjectLibraryAddElement<library::Symbol>* cmdAddToLibrary =
            new CmdProjectLibraryAddElement<library::Symbol>(mSchematic.getProject().getLibrary(), *sym);
        appendChild(cmdAddToLibrary); // can throw
    }

    // create child command to add a new symbol instance to the schematic
    mCmdAddToSchematic = new CmdSymbolInstanceAdd(mSchematic, mComponentInstance,
                                                  mSymbolItemUuid, mPosition, mAngle);
    appendChild(mCmdAddToSchematic); // can throw

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
