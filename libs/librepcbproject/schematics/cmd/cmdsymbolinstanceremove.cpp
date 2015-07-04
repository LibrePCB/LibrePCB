/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "cmdsymbolinstanceremove.h"
#include "../schematic.h"
#include "../items/si_symbol.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSymbolInstanceRemove::CmdSymbolInstanceRemove(Schematic& schematic, SI_Symbol& symbol,
                                                 UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove symbol"), parent),
    mSchematic(schematic), mSymbol(symbol)
{
}

CmdSymbolInstanceRemove::~CmdSymbolInstanceRemove() noexcept
{
    if (isExecuted())
        delete &mSymbol;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSymbolInstanceRemove::redo() throw (Exception)
{
    mSchematic.removeSymbol(mSymbol); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mSchematic.addSymbol(mSymbol);
        throw;
    }
}

void CmdSymbolInstanceRemove::undo() throw (Exception)
{
    mSchematic.addSymbol(mSymbol); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        mSchematic.removeSymbol(mSymbol);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
