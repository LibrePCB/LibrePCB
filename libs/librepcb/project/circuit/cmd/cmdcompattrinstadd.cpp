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
                                       const AttributeUnit* unit) noexcept :
    UndoCommand(tr("Add component attribute")),
    mComponentInstance(cmp), mKey(key), mType(type), mValue(value), mUnit(unit),
    mAttrInstance(nullptr)
{
}

CmdCompAttrInstAdd::~CmdCompAttrInstAdd() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCompAttrInstAdd::performExecute() throw (Exception)
{
    mAttrInstance = new ComponentAttributeInstance(mComponentInstance, mKey, mType, mValue, mUnit); // can throw

    performRedo(); // can throw

    return true;
}

void CmdCompAttrInstAdd::performUndo() throw (Exception)
{
    mComponentInstance.removeAttribute(*mAttrInstance); // can throw
}

void CmdCompAttrInstAdd::performRedo() throw (Exception)
{
    mComponentInstance.addAttribute(*mAttrInstance); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
