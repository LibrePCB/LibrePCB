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
#include "cmdgencompattrinstadd.h"
#include "../gencompinstance.h"
#include "../gencompattributeinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdGenCompAttrInstAdd::CmdGenCompAttrInstAdd(GenCompInstance& genComp, const QString& key,
                                             const AttributeType& type, const QString& value,
                                             const AttributeUnit* unit, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add component attribute"), parent),
    mGenCompInstance(genComp), mKey(key), mType(type), mValue(value), mUnit(unit),
    mAttrInstance(nullptr)
{
}

CmdGenCompAttrInstAdd::~CmdGenCompAttrInstAdd() noexcept
{
    if (!isExecuted())
        delete mAttrInstance;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdGenCompAttrInstAdd::redo() throw (Exception)
{
    if (!mAttrInstance) // only the first time
        mAttrInstance = new GenCompAttributeInstance(mKey, mType, mValue, mUnit); // throws an exception on error

    mGenCompInstance.addAttribute(*mAttrInstance); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        mGenCompInstance.removeAttribute(*mAttrInstance);
        throw;
    }
}

void CmdGenCompAttrInstAdd::undo() throw (Exception)
{
    mGenCompInstance.removeAttribute(*mAttrInstance); // throws an exception on error

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mGenCompInstance.addAttribute(*mAttrInstance);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
