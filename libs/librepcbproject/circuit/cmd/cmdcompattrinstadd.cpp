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
#include "cmdcompattrinstadd.h"
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

CmdCompAttrInstAdd::CmdCompAttrInstAdd(ComponentInstance& cmp, const QString& key,
                                       const AttributeType& type, const QString& value,
                                       const AttributeUnit* unit, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add component attribute"), parent),
    mComponentInstance(cmp), mKey(key), mType(type), mValue(value), mUnit(unit),
    mAttrInstance(nullptr)
{
}

CmdCompAttrInstAdd::~CmdCompAttrInstAdd() noexcept
{
    if (!isExecuted())
        delete mAttrInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdCompAttrInstAdd::redo() throw (Exception)
{
    if (!mAttrInstance) // only the first time
        mAttrInstance = new ComponentAttributeInstance(mKey, mType, mValue, mUnit); // throws an exception on error

    mComponentInstance.addAttribute(*mAttrInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mComponentInstance.removeAttribute(*mAttrInstance);
        throw;
    }
}

void CmdCompAttrInstAdd::undo() throw (Exception)
{
    mComponentInstance.removeAttribute(*mAttrInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mComponentInstance.addAttribute(*mAttrInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
