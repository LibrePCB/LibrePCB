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
#include "cmdcompattrinstedit.h"
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

CmdCompAttrInstEdit::CmdCompAttrInstEdit(ComponentInstance& cmp,
                                               ComponentAttributeInstance& attr,
                                               const AttributeType& newType,
                                               const QString& newValue,
                                               const AttributeUnit* newUnit,
                                               UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit component attribute"), parent),
    mComponentInstance(cmp), mAttrInst(attr),
    mOldType(&attr.getType()), mNewType(&newType),
    mOldValue(attr.getValue()), mNewValue(newValue),
    mOldUnit(attr.getUnit()), mNewUnit(newUnit)
{
}

CmdCompAttrInstEdit::~CmdCompAttrInstEdit() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdCompAttrInstEdit::redo() throw (Exception)
{
    try
    {
        mAttrInst.setTypeValueUnit(*mNewType, mNewValue, mNewUnit);
        UndoCommand::redo();
        emit mComponentInstance.attributesChanged();
    }
    catch (Exception &e)
    {
        mAttrInst.setTypeValueUnit(*mOldType, mOldValue, mOldUnit);
        throw;
    }
}

void CmdCompAttrInstEdit::undo() throw (Exception)
{
    try
    {
        mAttrInst.setTypeValueUnit(*mOldType, mOldValue, mOldUnit);
        UndoCommand::undo();
        emit mComponentInstance.attributesChanged();
    }
    catch (Exception& e)
    {
        mAttrInst.setTypeValueUnit(*mNewType, mNewValue, mNewUnit);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
