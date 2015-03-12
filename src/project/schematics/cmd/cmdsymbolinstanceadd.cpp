/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "../symbolinstance.h"
#include "../schematic.h"
#include "../../circuit/circuit.h"
#include "../../circuit/gencompinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSymbolInstanceAdd::CmdSymbolInstanceAdd(Schematic& schematic,
                                           GenCompInstance& genComp,
                                           const QUuid& symbolItem, const Point& position,
                                           const Angle& angle,
                                           UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add symbol instance"), parent),
    mSchematic(schematic), mSymbolInstance(nullptr)
{
    mSymbolInstance = mSchematic.createSymbol(genComp, symbolItem, position, angle); // throws an exception on error
}

CmdSymbolInstanceAdd::CmdSymbolInstanceAdd(SymbolInstance& symbol, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add symbol instance"), parent),
    mSchematic(symbol.getSchematic()), mSymbolInstance(&symbol)
{
}

CmdSymbolInstanceAdd::~CmdSymbolInstanceAdd() noexcept
{
    if (!isExecuted())
        delete mSymbolInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSymbolInstanceAdd::redo() throw (Exception)
{
    mSchematic.addSymbol(*mSymbolInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mSchematic.removeSymbol(*mSymbolInstance);
        throw;
    }
}

void CmdSymbolInstanceAdd::undo() throw (Exception)
{
    mSchematic.removeSymbol(*mSymbolInstance);  // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mSchematic.addSymbol(*mSymbolInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
