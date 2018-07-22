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
#include "si_netpoint.h"
#include "si_netline.h"
#include "si_symbol.h"
#include "si_symbolpin.h"
#include "si_netsegment.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../erc/ercmsg.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/scopeguardlist.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SI_NetPoint::SI_NetPoint(SI_NetSegment& segment, const SExpression& node) :
    SI_Base(segment.getSchematic()), mNetSegment(segment),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mSymbolPin(nullptr)
{
    const SExpression* symNode = node.tryGetChildByPath("sym");
    const SExpression* pinNode = node.tryGetChildByPath("pin");
    const SExpression* posNode = node.tryGetChildByPath("pos");

    if (symNode && pinNode && (!posNode)) {
        Uuid symbolUuid = symNode->getValueOfFirstChild<Uuid>();
        SI_Symbol* symbol = mSchematic.getSymbolByUuid(symbolUuid);
        if (!symbol) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid symbol UUID: \"%1\"")).arg(symbolUuid.toStr()));
        }
        Uuid pinUuid = pinNode->getValueOfFirstChild<Uuid>();
        mSymbolPin = symbol->getPin(pinUuid);
        if (!mSymbolPin) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid symbol pin UUID: \"%1\"")).arg(pinUuid.toStr()));
        }
        mPosition = mSymbolPin->getPosition();
    } else if (posNode && (!symNode) && (!pinNode)) {
        mPosition = Point(node.getChildByPath("pos"));
    } else {
        throw RuntimeError(__FILE__, __LINE__, tr("Invalid combination of sym/pin/pos nodes."));
    }

    init();
}

SI_NetPoint::SI_NetPoint(SI_NetSegment& segment, const Point& position) :
    SI_Base(segment.getSchematic()), mNetSegment(segment), mUuid(Uuid::createRandom()),
    mPosition(position), mSymbolPin(nullptr)
{
    init();
}

SI_NetPoint::SI_NetPoint(SI_NetSegment& segment, SI_SymbolPin& pin) :
    SI_Base(segment.getSchematic()), mNetSegment(segment), mUuid(Uuid::createRandom()),
    mPosition(pin.getPosition()), mSymbolPin(&pin)
{
    init();
}

void SI_NetPoint::init()
{
    // create the graphics item
    mGraphicsItem.reset(new SGI_NetPoint(*this));
    mGraphicsItem->setPos(mPosition.toPxQPointF());

    // create ERC messages
    mErcMsgDeadNetPoint.reset(new ErcMsg(mSchematic.getProject(), *this,
        mUuid.toStr(), "Dead", ErcMsg::ErcMsgType_t::SchematicError,
        QString(tr("Dead net point in schematic page \"%1\": %2"))
        .arg(mSchematic.getName()).arg(mUuid.toStr())));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SI_NetPoint::~SI_NetPoint() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool SI_NetPoint::isVisibleJunction() const noexcept
{
    if (mRegisteredLines.count() > 2) {
        return true;
    } else if ((mRegisteredLines.count() > 1) && isAttachedToPin()) {
        return true;
    } else {
        return false;
    }
}

bool SI_NetPoint::isOpenLineEnd() const noexcept
{
    return ((mRegisteredLines.count() <= 1) && (!isAttachedToPin()));
}

NetSignal& SI_NetPoint::getNetSignalOfNetSegment() const noexcept
{
    return mNetSegment.getNetSignal();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SI_NetPoint::setPinToAttach(SI_SymbolPin* pin)
{
    if (pin == mSymbolPin) {
        return;
    }
    if ((isUsed()) || ((pin) && (pin->getSchematic() != getSchematic()))) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToSchematic()) {
        ScopeGuardList sgl;
        if (mSymbolPin) {
            // detach from current pin
            mSymbolPin->unregisterNetPoint(*this); // can throw
            sgl.add([&](){mSymbolPin->registerNetPoint(*this);});
        }
        if (pin) {
            // attach to new pin
            if (pin->getCompSigInstNetSignal() != &mNetSegment.getNetSignal()) {
                throw LogicError(__FILE__, __LINE__);
            }
            pin->registerNetPoint(*this); // can throw
            sgl.add([&](){pin->unregisterNetPoint(*this);});
            setPosition(pin->getPosition());
        }
        sgl.dismiss();
    }
    mSymbolPin = pin;
    mGraphicsItem->updateCacheAndRepaint();
}

void SI_NetPoint::setPosition(const Point& position) noexcept
{
    if (position != mPosition) {
        mPosition = position;
        mGraphicsItem->setPos(mPosition.toPxQPointF());
        updateLines();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_NetPoint::addToSchematic()
{
    if (isAddedToSchematic() || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }

    if (isAttachedToPin()) {
        // check if netsignal is correct (would be a bug if not)
        if (mSymbolPin->getCompSigInstNetSignal() != &mNetSegment.getNetSignal()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mSymbolPin->registerNetPoint(*this); // can throw
    }

    mHighlightChangedConnection = connect(&getNetSignalOfNetSegment(),
                                          &NetSignal::highlightedChanged,
                                          [this](){mGraphicsItem->update();});
    mErcMsgDeadNetPoint->setVisible(true);
    SI_Base::addToSchematic(mGraphicsItem.data());
}

void SI_NetPoint::removeFromSchematic()
{
    if ((!isAddedToSchematic()) || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }

    if (isAttachedToPin()) {
        // check if netsignal is correct (would be a bug if not)
        if (mSymbolPin->getCompSigInstNetSignal() != &mNetSegment.getNetSignal()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mSymbolPin->unregisterNetPoint(*this); // can throw
    }

    disconnect(mHighlightChangedConnection);
    mErcMsgDeadNetPoint->setVisible(false);
    SI_Base::removeFromSchematic(mGraphicsItem.data());
}

void SI_NetPoint::registerNetLine(SI_NetLine& netline)
{
    if ((!isAddedToSchematic()) || (mRegisteredLines.contains(&netline))
        || (netline.getSchematic() != mSchematic))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredLines.append(&netline);
    netline.updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mRegisteredLines.isEmpty());
}

void SI_NetPoint::unregisterNetLine(SI_NetLine& netline)
{
    if ((!isAddedToSchematic()) || (!mRegisteredLines.contains(&netline))) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredLines.removeOne(&netline);
    netline.updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mRegisteredLines.isEmpty());
}

void SI_NetPoint::updateLines() const noexcept
{
    foreach (SI_NetLine* line, mRegisteredLines) {
        line->updateLine();
    }
}

void SI_NetPoint::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    if (isAttachedToPin()) {
        root.appendChild("sym", mSymbolPin->getSymbol().getUuid(), true);
        root.appendChild("pin", mSymbolPin->getLibPinUuid(), false);
    } else {
        root.appendChild(mPosition.serializeToDomElement("pos"), true);
    }
}

/*****************************************************************************************
 *  Inherited from SI_Base
 ****************************************************************************************/

QPainterPath SI_NetPoint::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->shape().translated(mPosition.toPxQPointF());
}

void SI_NetPoint::setSelected(bool selected) noexcept
{
    SI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SI_NetPoint::checkAttributesValidity() const noexcept
{
    if (isAttachedToPin() && (&mNetSegment.getNetSignal() != mSymbolPin->getCompSigInstNetSignal())) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
