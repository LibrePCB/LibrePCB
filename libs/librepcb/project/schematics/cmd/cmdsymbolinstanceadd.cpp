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
#include "cmdsymbolinstanceadd.h"
#include "../items/si_symbol.h"
#include "../schematic.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSymbolInstanceAdd::CmdSymbolInstanceAdd(Schematic& schematic,
        ComponentInstance& cmpInstance, const Uuid& symbolItem, const Point& position,
        const Angle& angle) noexcept :
    UndoCommand(tr("Add symbol instance")),
    mSchematic(schematic), mComponentInstance(&cmpInstance), mSymbolItemUuid(symbolItem),
    mPosition(position), mAngle(angle), mSymbolInstance(nullptr)
{
}

CmdSymbolInstanceAdd::CmdSymbolInstanceAdd(SI_Symbol& symbol) noexcept :
    UndoCommand(tr("Add symbol instance")),
    mSchematic(symbol.getSchematic()), mComponentInstance(nullptr), mSymbolItemUuid(),
    mPosition(), mAngle(), mSymbolInstance(&symbol)
{
}

CmdSymbolInstanceAdd::~CmdSymbolInstanceAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdSymbolInstanceAdd::performExecute() throw (Exception)
{
    if (!mSymbolInstance) {
        // create new symbol instance
        mSymbolInstance = new SI_Symbol(mSchematic, *mComponentInstance, mSymbolItemUuid,
                                        mPosition, mAngle); // can throw
    }

    performRedo(); // can throw

    return true;
}

void CmdSymbolInstanceAdd::performUndo() throw (Exception)
{
    mSchematic.removeSymbol(*mSymbolInstance); // can throw
}

void CmdSymbolInstanceAdd::performRedo() throw (Exception)
{
    mSchematic.addSymbol(*mSymbolInstance); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
