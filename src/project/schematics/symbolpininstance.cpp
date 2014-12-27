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
#include "../erc/ercmsg.h"
#include "schematic.h"
#include "../project.h"
#include "../circuit/circuit.h"
#include "../../library/gencompsymbvaritem.h"
#include "../circuit/netsignal.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPinInstance::SymbolPinInstance(SymbolInstance& symbolInstance, const QUuid& pinUuid) :
    QObject(0), mCircuit(symbolInstance.getSchematic().getProject().getCircuit()),
    mSymbolInstance(symbolInstance), mSymbolPin(nullptr), mGenCompSignal(nullptr),
    mGenCompSignalInstance(nullptr), mRegisteredSchematicNetPoint(nullptr),
    mRegisteredPinGraphicsItem(nullptr)
{
    // read attributes
    mSymbolPin = mSymbolInstance.getSymbol().getPinByUuid(pinUuid);
    if (!mSymbolPin)
    {
        throw RuntimeError(__FILE__, __LINE__, pinUuid.toString(),
            QString(tr("Invalid symbol pin UUID: \"%1\"")).arg(pinUuid.toString()));
    }
    QUuid genCompSignalUuid = mSymbolInstance.getGenCompSymbVarItem().getSignalOfPin(pinUuid);
    mGenCompSignalInstance = mSymbolInstance.getGenCompInstance().getSignalInstance(genCompSignalUuid);
    mGenCompSignal = mSymbolInstance.getGenCompInstance().getGenComp().getSignalByUuid(genCompSignalUuid);

    // create ERC messages
    mErcMsgUnconnectedRequiredPin.reset(new ErcMsg(mCircuit.getProject(), *this,
        QString("%1/%2").arg(mSymbolInstance.getUuid().toString()).arg(mSymbolPin->getUuid().toString()),
        "UnconnectedRequiredPin", ErcMsg::ErcMsgType_t::SchematicError,
        QString(tr("Unconnected pin: \"%1\" of component \"%2\""))
        .arg(getDisplayText(true, true)).arg(mSymbolInstance.getGenCompInstance().getName())));
}

SymbolPinInstance::~SymbolPinInstance()
{
    Q_ASSERT(mRegisteredSchematicNetPoint == nullptr);
    Q_ASSERT(mRegisteredPinGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const QUuid& SymbolPinInstance::getLibPinUuid() const noexcept
{
    return mSymbolPin->getUuid();
}

Point SymbolPinInstance::getPosition() const noexcept
{
    return mSymbolInstance.mapToScene(mSymbolPin->getPosition());
}

QString SymbolPinInstance::getDisplayText(bool returnGenCompSignalNameIfEmpty,
                                          bool returnPinNameIfEmpty) const noexcept
{
    QString text;
    switch (mSymbolInstance.getGenCompSymbVarItem().getDisplayTypeOfPin(mSymbolPin->getUuid()))
    {
        case library::GenCompSymbVarItem::PinDisplayType_t::PinName:
            text = mSymbolPin->getName(); break;
        case library::GenCompSymbVarItem::PinDisplayType_t::GenCompSignal:
            if (mGenCompSignal) text = mGenCompSignal->getName(); break;
        case library::GenCompSymbVarItem::PinDisplayType_t::NetSignal:
            if (mGenCompSignalInstance->getNetSignal()) text = mGenCompSignalInstance->getNetSignal()->getName(); break;
        default: break;
    }
    if (text.isEmpty() && returnGenCompSignalNameIfEmpty && mGenCompSignal)
        text = mGenCompSignal->getName();
    if (text.isEmpty() && returnPinNameIfEmpty)
        text = mSymbolPin->getName();
    return text;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolPinInstance::updateNetPointPosition() noexcept
{
    if (mRegisteredSchematicNetPoint)
        mRegisteredSchematicNetPoint->setPosition(getPosition());
}

void SymbolPinInstance::registerNetPoint(SchematicNetPoint* netpoint)
{
    Q_ASSERT(mRegisteredSchematicNetPoint == nullptr);
    mRegisteredSchematicNetPoint = netpoint;
    mErcMsgUnconnectedRequiredPin->setVisible((mGenCompSignal->isRequired()) && (!mRegisteredSchematicNetPoint));
}

void SymbolPinInstance::unregisterNetPoint(SchematicNetPoint* netpoint)
{
    Q_UNUSED(netpoint); // to avoid compiler warning in release mode
    Q_ASSERT(mRegisteredSchematicNetPoint == netpoint);
    mRegisteredSchematicNetPoint = nullptr;
    mErcMsgUnconnectedRequiredPin->setVisible((mGenCompSignal->isRequired()) && (!mRegisteredSchematicNetPoint));
}

void SymbolPinInstance::registerPinGraphicsItem(library::SymbolPinGraphicsItem* item)
{
    Q_ASSERT(mRegisteredPinGraphicsItem == nullptr);
    mRegisteredPinGraphicsItem = item;
}

void SymbolPinInstance::unregisterPinGraphicsItem(library::SymbolPinGraphicsItem* item)
{
    Q_UNUSED(item); // to avoid compiler warning in release mode
    Q_ASSERT(mRegisteredPinGraphicsItem == item);
    mRegisteredPinGraphicsItem = nullptr;
}

void SymbolPinInstance::addToSchematic() noexcept
{
    Q_ASSERT(mRegisteredSchematicNetPoint == nullptr);
    mGenCompSignalInstance->registerSymbolPinInstance(this);
    mErcMsgUnconnectedRequiredPin->setVisible((mGenCompSignal->isRequired()) && (!mRegisteredSchematicNetPoint));
}

void SymbolPinInstance::removeFromSchematic() noexcept
{
    Q_ASSERT(mRegisteredSchematicNetPoint == nullptr);
    mGenCompSignalInstance->unregisterSymbolPinInstance(this);
    mErcMsgUnconnectedRequiredPin->setVisible(false);
}

bool SymbolPinInstance::save(bool toOriginal, QStringList& errors) noexcept
{
    Q_UNUSED(toOriginal);
    Q_UNUSED(errors);
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
