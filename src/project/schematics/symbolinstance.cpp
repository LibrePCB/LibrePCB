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
#include "symbolinstance.h"
#include "schematic.h"
#include "../project.h"
#include "../circuit/circuit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolInstance::SymbolInstance(Schematic& schematic, const QDomElement& domElement)
                               throw (Exception) :
    QObject(0), mSchematic(schematic), mDomElement(domElement)
{
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid symbol instance UUID: \"%1\""))
            .arg(mDomElement.attribute("uuid")));
    }

    QString gcUuid = mDomElement.attribute("gen_comp_instance");
    mGenCompInstance = schematic.getProject().getCircuit().getGenCompInstanceByUuid(gcUuid);
    if (!mGenCompInstance)
    {
        throw RuntimeError(__FILE__, __LINE__, gcUuid,
            QString(tr("No generic component with the UUID \"%1\" found in the circuit!"))
                           .arg(gcUuid));
    }
}

SymbolInstance::~SymbolInstance() noexcept
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
