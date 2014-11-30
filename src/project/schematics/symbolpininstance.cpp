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
#include "symbolpininstance.h"
#include "symbolinstance.h"
#include "../../library/symbol.h"
#include "../../library/symbolpin.h"
#include "../circuit/genericcomponentinstance.h"
#include "../../library/genericcomponent.h"
#include "schematicnetpoint.h"
#include "../circuit/gencompsignalinstance.h"
#include "../../library/symbolpingraphicsitem.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPinInstance::SymbolPinInstance(SymbolInstance& symbolInstance, const QUuid& pinUuid) :
    QObject(0), mSymbolInstance(symbolInstance), mSymbolPin(0), mGenCompSignal(0),
    mGenCompSignalInstance(0), mRegisteredSchematicNetPoint(0), mRegisteredPinGraphicsItem(0)
{
    mSymbolPin = mSymbolInstance.getSymbol().getPinByUuid(pinUuid);
    if (!mSymbolPin)
    {
        throw RuntimeError(__FILE__, __LINE__, pinUuid.toString(),
            QString(tr("Invalid symbol pin UUID: \"%1\"")).arg(pinUuid.toString()));
    }

    QUuid genCompSignalUuid = mSymbolInstance.getGenCompSymbVarItem().getSignalOfPin(pinUuid);
    mGenCompSignalInstance = mSymbolInstance.getGenCompInstance().getSignalInstance(genCompSignalUuid);
    mGenCompSignal = mSymbolInstance.getGenCompInstance().getGenComp().getSignalByUuid(genCompSignalUuid);
}

SymbolPinInstance::~SymbolPinInstance()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolPinInstance::updateLines() noexcept
{
    if (mRegisteredSchematicNetPoint)
        mRegisteredSchematicNetPoint->updateLines();
}

void SymbolPinInstance::registerNetPoint(SchematicNetPoint* netpoint,
                                         SchematicNetPointGraphicsItem* item)
{
    Q_ASSERT(mRegisteredSchematicNetPoint == 0);
    mRegisteredSchematicNetPoint = netpoint;
    Q_ASSERT(mRegisteredPinGraphicsItem);
    item->setParentItem(mRegisteredPinGraphicsItem);
    item->setPos(0, 0);
}

void SymbolPinInstance::unregisterNetPoint(SchematicNetPoint* netpoint,
                                           SchematicNetPointGraphicsItem* item)
{
    Q_UNUSED(netpoint); // to avoid compiler warning in release mode
    Q_ASSERT(mRegisteredSchematicNetPoint == netpoint);
    mRegisteredSchematicNetPoint = 0;
    item->setParentItem(0);
    item->setPos(0, 0);
}

void SymbolPinInstance::registerPinGraphicsItem(library::SymbolPinGraphicsItem* item)
{
    Q_ASSERT(mRegisteredPinGraphicsItem == 0);
    mRegisteredPinGraphicsItem = item;
}

void SymbolPinInstance::unregisterPinGraphicsItem(library::SymbolPinGraphicsItem* item)
{
    Q_UNUSED(item); // to avoid compiler warning in release mode
    Q_ASSERT(mRegisteredPinGraphicsItem == item);
    mRegisteredPinGraphicsItem = 0;
}

void SymbolPinInstance::addToSchematic() noexcept
{
    mGenCompSignalInstance->registerSymbolPinInstance(this);
}

void SymbolPinInstance::removeFromSchematic() noexcept
{
    mGenCompSignalInstance->unregisterSymbolPinInstance(this);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
