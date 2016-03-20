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
    SI_Base(symbol.getSchematic()), mSymbol(symbol), mSymbolPin(nullptr),
    mPinSignalMapItem(nullptr), mComponentSignalInstance(nullptr),
    mRegisteredNetPoint(nullptr)
{
    // read attributes
    mSymbolPin = mSymbol.getLibSymbol().getPinByUuid(pinUuid);
    if (!mSymbolPin) {
        throw RuntimeError(__FILE__, __LINE__, pinUuid.toStr(),
            QString(tr("Invalid symbol pin UUID: \"%1\"")).arg(pinUuid.toStr()));
    }
    mPinSignalMapItem = mSymbol.getCompSymbVarItem().getPinSignalMapItemOfPin(pinUuid);
    if (!mPinSignalMapItem) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Pin \"%1\" not found in pin-signal-map of symbol instance \"%2\"."))
            .arg(pinUuid.toStr(), symbol.getUuid().toStr()));
    }
    Uuid cmpSignalUuid = mPinSignalMapItem->getSignalUuid();
    mComponentSignalInstance = mSymbol.getComponentInstance().getSignalInstance(cmpSignalUuid);

    mGraphicsItem.reset(new SGI_SymbolPin(*this));
    updatePosition();

    // create ERC messages
    mErcMsgUnconnectedRequiredPin.reset(new ErcMsg(mSchematic.getProject(), *this,
        QString("%1/%2").arg(mSymbol.getUuid().toStr()).arg(mSymbolPin->getUuid().toStr()),
        "UnconnectedRequiredPin", ErcMsg::ErcMsgType_t::SchematicError));
    updateErcMessages();
}

SI_SymbolPin::~SI_SymbolPin()
{
    Q_ASSERT(!isUsed());
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

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
            if (mComponentSignalInstance) {
                text = mComponentSignalInstance->getCompSignal().getName();
            }
            break;
        case PinDisplayType_t::NET_SIGNAL:
            if (mComponentSignalInstance) {
                if (mComponentSignalInstance->getNetSignal()) {
                    text = mComponentSignalInstance->getNetSignal()->getName();
                }
            }
            break;
        default: break;
    }
    if (text.isEmpty() && returnCmpSignalNameIfEmpty && mComponentSignalInstance)
        text = mComponentSignalInstance->getCompSignal().getName();
    if (text.isEmpty() && returnPinNameIfEmpty)
        text = mSymbolPin->getName();
    return text;
}

NetSignal* SI_SymbolPin::getCompSigInstNetSignal() const noexcept
{
    if (mComponentSignalInstance) {
        return mComponentSignalInstance->getNetSignal();
    } else {
        return nullptr;
    }
}

bool SI_SymbolPin::isRequired() const noexcept
{
    if (mComponentSignalInstance) {
        return mComponentSignalInstance->getCompSignal().isRequired();
    } else {
        return false;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_SymbolPin::addToSchematic(GraphicsScene& scene) throw (Exception)
{
    if (isAddedToSchematic() || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (mComponentSignalInstance) {
        mComponentSignalInstance->registerSymbolPin(*this); // can throw
    }
    if (getCompSigInstNetSignal()) {
        mHighlightChangedConnection = connect(getCompSigInstNetSignal(), &NetSignal::highlightedChanged,
                                              [this](){mGraphicsItem->update();});
    }
    SI_Base::addToSchematic(scene, *mGraphicsItem);
    updateErcMessages();
}

void SI_SymbolPin::removeFromSchematic(GraphicsScene& scene) throw (Exception)
{
    if ((!isAddedToSchematic()) || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (mComponentSignalInstance) {
        mComponentSignalInstance->unregisterSymbolPin(*this); // can throw
    }
    if (getCompSigInstNetSignal()) {
        disconnect(mHighlightChangedConnection);
    }
    SI_Base::removeFromSchematic(scene, *mGraphicsItem);
    updateErcMessages();
}

void SI_SymbolPin::registerNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    if ((!isAddedToSchematic()) || (!mComponentSignalInstance) || (mRegisteredNetPoint)
        || (netpoint.getSchematic() != mSchematic)
        || (&netpoint.getNetSignal() != mComponentSignalInstance->getNetSignal()))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoint = &netpoint;
    updateErcMessages();
}

void SI_SymbolPin::unregisterNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    if ((!isAddedToSchematic()) || (!mComponentSignalInstance)
        || (mRegisteredNetPoint != &netpoint)
        || (&netpoint.getNetSignal() != mComponentSignalInstance->getNetSignal()))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoint = nullptr;
    updateErcMessages();
}

void SI_SymbolPin::updatePosition() noexcept
{
    mPosition = mSymbol.mapToScene(mSymbolPin->getPosition());
    mRotation = mSymbol.getRotation() + mSymbolPin->getRotation();
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(-mRotation.toDeg());
    mGraphicsItem->updateCacheAndRepaint();
    if (mRegisteredNetPoint) {
        mRegisteredNetPoint->setPosition(mPosition);
    }
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

    mErcMsgUnconnectedRequiredPin->setVisible(isAddedToSchematic() && isRequired()
                                              && (!mRegisteredNetPoint));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
