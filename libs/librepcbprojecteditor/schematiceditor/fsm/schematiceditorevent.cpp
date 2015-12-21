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
#include "schematiceditorevent.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include <librepcbproject/schematics/schematic.h>


namespace project {

/*****************************************************************************************
 *  Class SEE_Base
 ****************************************************************************************/

SEE_Base::SEE_Base(EventType_t type) :
    mType(type), mAccepted(false)
{
}

SEE_Base::~SEE_Base()
{
}

/*****************************************************************************************
 *  Class SEE_SetAddComponentParams
 ****************************************************************************************/

SEE_StartAddComponent::SEE_StartAddComponent() :
    SEE_Base(EventType_t::StartAddComponent), mGenCompUuid(), mSymbVarUuid()
{
}

SEE_StartAddComponent::SEE_StartAddComponent(const Uuid& genComp, const Uuid& symbVar) :
    SEE_Base(EventType_t::StartAddComponent), mGenCompUuid(genComp), mSymbVarUuid(symbVar)
{
}

SEE_StartAddComponent::~SEE_StartAddComponent()
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
