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
#include "si_symbolpin.h"
#include "si_symbol.h"
#include <librepcblibrary/sym/symbol.h>
#include <librepcblibrary/sym/symbolpin.h>
#include "../../circuit/componentinstance.h"
#include <librepcblibrary/cmp/component.h>
#include "si_netpoint.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../erc/ercmsg.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include <librepcblibrary/cmp/componentsymbolvariantitem.h>
#include "../../circuit/netsignal.h"
#include "../../settings/projectsettings.h"
#include <librepcbcommon/graphics/graphicsscene.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SI_SymbolPin::SI_SymbolPin(SI_Symbol& symbol, const Uuid& pinUuid) :
    SI_Base(), mCircuit(symbol.getSchematic().getProject().getCircuit()),
    mSymbol(symbol), mSymbolPin(nullptr), mComponentSignal(nullptr),
    mPinSignalMapItem(nullptr), mComponentSignalInstance(nullptr),
    mAddedToSchematic(false), mRegisteredNetPoint(nullptr), mGraphicsItem(nullptr)
{
    // read attributes
    mSymbolPin = mSymbol.getLibSymbol().getPinByUuid(pinUuid);
    if (!mSymbolPin)
    {
        throw RuntimeError(__FILE__, __LINE__, pinUuid.toStr(),
            QString(tr("Invalid symbol pin UUID: \"%1\"")).arg(pinUuid.toStr()));
    }
    mPinSignalMapItem = mSymbol.getCompSymbVarItem().getPinSignalMapItemOfPin(pinUuid);
    if (!mPinSignalMapItem)
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Pin \"%1\" not found in pin-signal-map of symbol instance \"%2\"."))
            .arg(pinUuid.toStr(), symbol.getUuid().toStr()));
    }
    Uuid cmpSignalUuid = mPinSignalMapItem->getSignalUuid();
    mComponentSignalInstance = mSymbol.getComponentInstance().getSignalInstance(cmpSignalUuid);
    mComponentSignal = mSymbol.getComponentInstance().getLibComponent().getSignalByUuid(cmpSignalUuid);

    mGraphicsItem = new SGI_SymbolPin(*this);
    updatePosition();

    // create ERC messages
    mErcMsgUnconnectedRequiredPin.reset(new ErcMsg(mCircuit.getProject(), *this,
        QString("%1/%2").arg(mSymbol.getUuid().toStr()).arg(mSymbolPin->getUuid().toStr()),
        "UnconnectedRequiredPin", ErcMsg::ErcMsgType_t::SchematicError));
    updateErcMessages();
}

SI_SymbolPin::~SI_SymbolPin()
{
    Q_ASSERT(mRegisteredNetPoint == nullptr);
    delete mGraphicsItem;       mGraphicsItem = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Project& SI_SymbolPin::getProject() const noexcept
{
    return mSymbol.getProject();
}

Schematic& SI_SymbolPin::getSchematic() const noexcept
{
    return mSymbol.getSchematic();
}

const Uuid& SI_SymbolPin::getLibPinUuid() const noexcept
{
    return mSymbolPin->getUuid();
}

QString SI_SymbolPin::getDisplayText(bool returnCmpSignalNameIfEmpty,
                                     bool returnPinNameIfEmpty) const noexcept
{
    QString text;
    using PinDisplayType_t = library::ComponentPinSignalMapItem::PinDisplayType_t;
    switch (mPinSignalMapItem->getDisplayType())
    {
        case PinDisplayType_t::PIN_NAME:
            text = mSymbolPin->getName(); break;
        case PinDisplayType_t::COMPONENT_SIGNAL:
            if (mComponentSignal) text = mComponentSignal->getName(); break;
        case PinDisplayType_t::NET_SIGNAL:
            if (mComponentSignalInstance->getNetSignal()) text = mComponentSignalInstance->getNetSignal()->getName(); break;
        default: break;
    }
    if (text.isEmpty() && returnCmpSignalNameIfEmpty && mComponentSignal)
        text = mComponentSignal->getName();
    if (text.isEmpty() && returnPinNameIfEmpty)
        text = mSymbolPin->getName();
    return text;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_SymbolPin::updatePosition() noexcept
{
    mPosition = mSymbol.mapToScene(mSymbolPin->getPosition());
    mRotation = mSymbol.getRotation() + mSymbolPin->getRotation();
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(-mRotation.toDeg());
    mGraphicsItem->updateCacheAndRepaint();
    if (mRegisteredNetPoint)
        mRegisteredNetPoint->setPosition(mPosition);
}

void SI_SymbolPin::registerNetPoint(SI_NetPoint& netpoint)
{
    Q_ASSERT(mRegisteredNetPoint == nullptr);
    mRegisteredNetPoint = &netpoint;
    updateErcMessages();
}

void SI_SymbolPin::unregisterNetPoint(SI_NetPoint& netpoint)
{
    Q_UNUSED(netpoint); // to avoid compiler warning in release mode
    Q_ASSERT(mRegisteredNetPoint == &netpoint);
    mRegisteredNetPoint = nullptr;
    updateErcMessages();
}

void SI_SymbolPin::addToSchematic(GraphicsScene& scene) noexcept
{
    Q_ASSERT(mAddedToSchematic == false);
    Q_ASSERT(mRegisteredNetPoint == nullptr);
    mComponentSignalInstance->registerSymbolPin(*this);
    scene.addItem(*mGraphicsItem);
    mAddedToSchematic = true;
    updateErcMessages();
}

void SI_SymbolPin::removeFromSchematic(GraphicsScene& scene) noexcept
{
    Q_ASSERT(mAddedToSchematic == true);
    Q_ASSERT(mRegisteredNetPoint == nullptr);
    mComponentSignalInstance->unregisterSymbolPin(*this);
    scene.removeItem(*mGraphicsItem);
    mAddedToSchematic = false;
    updateErcMessages();
}

/*****************************************************************************************
 *  Inherited from SI_Base
 ****************************************************************************************/

QPainterPath SI_SymbolPin::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void SI_SymbolPin::setSelected(bool selected) noexcept
{
    SI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void SI_SymbolPin::updateErcMessages() noexcept
{
    mErcMsgUnconnectedRequiredPin->setMsg(
        QString(tr("Unconnected pin: \"%1\" of symbol \"%2\""))
        .arg(getDisplayText(true, true)).arg(mSymbol.getName()));

    mErcMsgUnconnectedRequiredPin->setVisible((mAddedToSchematic)
        && (mComponentSignal->isRequired()) && (!mRegisteredNetPoint));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
