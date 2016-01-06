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
#include "cmdcompattrinstremove.h"
#include "../componentinstance.h"
#include "../componentattributeinstance.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCompAttrInstRemove::CmdCompAttrInstRemove(ComponentInstance& cmp,
                                                   ComponentAttributeInstance& attr,
                                                   UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Remove component attribute"), parent),
    mComponentInstance(cmp), mAttrInstance(attr)
{
}

CmdCompAttrInstRemove::~CmdCompAttrInstRemove() noexcept
{
    if (isExecuted())
        delete &mAttrInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdCompAttrInstRemove::redo() throw (Exception)
{
    mComponentInstance.removeAttribute(mAttrInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mComponentInstance.addAttribute(mAttrInstance);
        throw;
    }
}

void CmdCompAttrInstRemove::undo() throw (Exception)
{
    mComponentInstance.addAttribute(mAttrInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mComponentInstance.removeAttribute(mAttrInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
