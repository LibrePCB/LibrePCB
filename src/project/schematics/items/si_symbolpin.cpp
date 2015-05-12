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
#include "si_symbolpin.h"
#include "si_symbol.h"
#include "../../../library/sym/symbol.h"
#include "../../../library/sym/symbolpin.h"
#include "../../circuit/gencompinstance.h"
#include "../../../library/gencmp/genericcomponent.h"
#include "si_netpoint.h"
#include "../../circuit/gencompsignalinstance.h"
#include "../../erc/ercmsg.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../../library/gencmp/gencompsymbvaritem.h"
#include "../../circuit/netsignal.h"
#include "../../settings/projectsettings.h"
#include "../../../common/graphics/graphicsscene.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SI_SymbolPin::SI_SymbolPin(SI_Symbol& symbol, const QUuid& pinUuid) :
    SI_Base(), mCircuit(symbol.getSchematic().getProject().getCircuit()),
    mSymbol(symbol), mSymbolPin(nullptr), mGenCompSignal(nullptr),
    mGenCompSignalInstance(nullptr), mAddedToSchematic(false),
    mRegisteredNetPoint(nullptr), mGraphicsItem(nullptr)
{
    // read attributes
    mSymbolPin = mSymbol.getLibSymbol().getPinByUuid(pinUuid);
    if (!mSymbolPin)
    {
        throw RuntimeError(__FILE__, __LINE__, pinUuid.toString(),
            QString(tr("Invalid symbol pin UUID: \"%1\"")).arg(pinUuid.toString()));
    }
    QUuid genCompSignalUuid = mSymbol.getGenCompSymbVarItem().getSignalOfPin(pinUuid);
    mGenCompSignalInstance = mSymbol.getGenCompInstance().getSignalInstance(genCompSignalUuid);
    mGenCompSignal = mSymbol.getGenCompInstance().getGenComp().getSignalByUuid(genCompSignalUuid);

    mGraphicsItem = new SGI_SymbolPin(*this);
    updatePosition();

    // create ERC messages
    mErcMsgUnconnectedRequiredPin.reset(new ErcMsg(mCircuit.getProject(), *this,
        QString("%1/%2").arg(mSymbol.getUuid().toString()).arg(mSymbolPin->getUuid().toString()),
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

const QUuid& SI_SymbolPin::getLibPinUuid() const noexcept
{
    return mSymbolPin->getUuid();
}

QString SI_SymbolPin::getDisplayText(bool returnGenCompSignalNameIfEmpty,
                                     bool returnPinNameIfEmpty) const noexcept
{
    const QStringList& localeOrder = mCircuit.getProject().getSettings().getLocaleOrder();

    QString text;
    switch (mSymbol.getGenCompSymbVarItem().getDisplayTypeOfPin(mSymbolPin->getUuid()))
    {
        case library::GenCompSymbVarItem::PinDisplayType_t::PinName:
            text = mSymbolPin->getName(localeOrder); break;
        case library::GenCompSymbVarItem::PinDisplayType_t::GenCompSignal:
            if (mGenCompSignal) text = mGenCompSignal->getName(localeOrder); break;
        case library::GenCompSymbVarItem::PinDisplayType_t::NetSignal:
            if (mGenCompSignalInstance->getNetSignal()) text = mGenCompSignalInstance->getNetSignal()->getName(); break;
        default: break;
    }
    if (text.isEmpty() && returnGenCompSignalNameIfEmpty && mGenCompSignal)
        text = mGenCompSignal->getName(localeOrder);
    if (text.isEmpty() && returnPinNameIfEmpty)
        text = mSymbolPin->getName(localeOrder);
    return text;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_SymbolPin::updatePosition() noexcept
{
    mPosition = mSymbol.mapToScene(mSymbolPin->getPosition());
    mAngle = mSymbol.getAngle() + mSymbolPin->getAngle();
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(mAngle.toDeg());
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
    mGenCompSignalInstance->registerSymbolPin(*this);
    scene.addItem(*mGraphicsItem);
    mAddedToSchematic = true;
    updateErcMessages();
}

void SI_SymbolPin::removeFromSchematic(GraphicsScene& scene) noexcept
{
    Q_ASSERT(mAddedToSchematic == true);
    Q_ASSERT(mRegisteredNetPoint == nullptr);
    mGenCompSignalInstance->unregisterSymbolPin(*this);
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
        && (mGenCompSignal->isRequired()) && (!mRegisteredNetPoint));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
