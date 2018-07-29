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
#include "bi_netpoint.h"
#include "bi_netline.h"
#include "bi_footprint.h"
#include "bi_footprintpad.h"
#include "bi_netsegment.h"
#include "bi_device.h"
#include "bi_via.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../erc/ercmsg.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/common/scopeguardlist.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, const BI_NetPoint& other,
                         BI_FootprintPad* pad, BI_Via* via) :
    BI_Base(segment.getBoard()), mNetSegment(segment), mUuid(Uuid::createRandom()),
    mPosition(other.mPosition), mLayer(nullptr), mFootprintPad(pad), mVia(via)
{
    mLayer = mBoard.getLayerStack().getLayer(other.getLayer().getName());

    if (((other.getFootprintPad() == nullptr) != (mFootprintPad == nullptr))
        || ((other.getVia() == nullptr) != (mVia == nullptr)) || (!mLayer))
    {
        throw LogicError(__FILE__, __LINE__);
    }

    init();
}

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, const SExpression& node) :
    BI_Base(segment.getBoard()), mNetSegment(segment),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mLayer(nullptr),
    mFootprintPad(nullptr),
    mVia(nullptr)
{
    // read attributes
    QString layerName = node.getValueByPath<QString>("layer");
    mLayer = mBoard.getLayerStack().getLayer(layerName);
    if (!mLayer) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid board layer: \"%1\""))
            .arg(layerName));
    }

    const SExpression* viaNode = node.tryGetChildByPath("via");
    const SExpression* devNode = node.tryGetChildByPath("dev");
    const SExpression* padNode = node.tryGetChildByPath("pad");
    const SExpression* posNode = node.tryGetChildByPath("pos");

    if (viaNode && (!devNode) && (!padNode) && (!posNode)) {
        Uuid viaUuid = viaNode->getValueOfFirstChild<Uuid>();
        mVia = mNetSegment.getViaByUuid(viaUuid);
        if (!mVia) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid via UUID: \"%1\"")).arg(viaUuid.toStr()));
        }
        mPosition = mVia->getPosition();
    } else if (devNode && padNode && (!viaNode) && (!posNode)) {
        Uuid componentUuid = devNode->getValueOfFirstChild<Uuid>();
        BI_Device* device = mBoard.getDeviceInstanceByComponentUuid(componentUuid);
        if (!device) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid component UUID: \"%1\"")).arg(componentUuid.toStr()));
        }
        Uuid padUuid = padNode->getValueOfFirstChild<Uuid>();
        mFootprintPad = device->getFootprint().getPad(padUuid);
        if (!mFootprintPad) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid footprint pad UUID: \"%1\"")).arg(padUuid.toStr()));
        }
        mPosition = mFootprintPad->getPosition();
    } else if (posNode && (!viaNode) && (!devNode) && (!padNode)) {
        mPosition = Point(*posNode);
    } else {
        throw RuntimeError(__FILE__, __LINE__, tr("Invalid combination of sym/pin/pos nodes."));
    }

    init();
}

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, GraphicsLayer& layer, const Point& position) :
    BI_Base(segment.getBoard()), mNetSegment(segment), mUuid(Uuid::createRandom()),
    mPosition(position), mLayer(&layer), mFootprintPad(nullptr), mVia(nullptr)
{
    init();
}

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, GraphicsLayer& layer, BI_FootprintPad& pad) :
    BI_Base(segment.getBoard()), mNetSegment(segment), mUuid(Uuid::createRandom()),
    mPosition(pad.getPosition()), mLayer(&layer), mFootprintPad(&pad), mVia(nullptr)
{
    init();
}

BI_NetPoint::BI_NetPoint(BI_NetSegment& segment, GraphicsLayer& layer, BI_Via& via) :
    BI_Base(segment.getBoard()), mNetSegment(segment), mUuid(Uuid::createRandom()),
    mPosition(via.getPosition()), mLayer(&layer), mFootprintPad(nullptr), mVia(&via)
{
    init();
}

void BI_NetPoint::init()
{
    // check layer
    if (!mLayer->isCopperLayer()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The layer of netpoint \"%1\" is invalid (%2)."))
            .arg(mUuid.toStr()).arg(mLayer->getName()));
    }
    if (mFootprintPad) {
        if (!mFootprintPad->isOnLayer(mLayer->getName())) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The layer of netpoint \"%1\" is invalid (%2)."))
                .arg(mUuid.toStr()).arg(mLayer->getName()));
        }
    }

    // create the graphics item
    mGraphicsItem.reset(new BGI_NetPoint(*this));
    mGraphicsItem->setPos(mPosition.toPxQPointF());

    // create ERC messages
    mErcMsgDeadNetPoint.reset(new ErcMsg(mBoard.getProject(), *this,
        mUuid.toStr(), "Dead", ErcMsg::ErcMsgType_t::BoardError,
        QString(tr("Dead net point in board page \"%1\": %2"))
        .arg(*mBoard.getName()).arg(mUuid.toStr())));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_NetPoint::~BI_NetPoint() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

NetSignal& BI_NetPoint::getNetSignalOfNetSegment() const noexcept
{
    return mNetSegment.getNetSignal();
}

UnsignedLength BI_NetPoint::getMaxLineWidth() const noexcept
{
    UnsignedLength w(0);
    foreach (BI_NetLine* line, mRegisteredLines) {
        if (line->getWidth() > w) {
            w = positiveToUnsigned(line->getWidth());
        }
    }
    return w;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void BI_NetPoint::setLayer(GraphicsLayer& layer)
{
    if (&layer != mLayer) {
        if (isUsed() || isAttached() || (!layer.isCopperLayer())) {
            throw LogicError(__FILE__, __LINE__);
        }
        mLayer = &layer;
    }
}

void BI_NetPoint::setPadToAttach(BI_FootprintPad* pad)
{
    if (pad == mFootprintPad) {
        return;
    }
    if ((isUsed()) || (isAttachedToVia()) || ((pad) && (pad->getBoard() != getBoard()))) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
        ScopeGuardList sgl;
        if (isAttachedToPad()) {
            // detach from current pad
            mFootprintPad->unregisterNetPoint(*this); // can throw
            sgl.add([&](){mFootprintPad->registerNetPoint(*this);});
        }
        if (pad) {
            // attach to new pad
            if (pad->getCompSigInstNetSignal() != &mNetSegment.getNetSignal()) {
                throw LogicError(__FILE__, __LINE__);
            }
            pad->registerNetPoint(*this); // can throw
            sgl.add([&](){pad->unregisterNetPoint(*this);});
            setPosition(pad->getPosition());
        }
        mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
        sgl.dismiss();
    }
    mFootprintPad = pad;
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_NetPoint::setViaToAttach(BI_Via* via)
{
    if (via == mVia) {
        return;
    }
    if ((isUsed()) || (isAttachedToPad()) || ((via) && (via->getBoard() != getBoard()))) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
        ScopeGuardList sgl;
        if (isAttachedToVia()) {
            // detach from current via
            mVia->unregisterNetPoint(*this); // can throw
            sgl.add([&](){mVia->registerNetPoint(*this);});
        }
        if (via) {
            // attach to new via
            if (&via->getNetSegment() != &mNetSegment) {
                throw LogicError(__FILE__, __LINE__);
            }
            via->registerNetPoint(*this); // can throw
            sgl.add([&](){via->unregisterNetPoint(*this);});
            setPosition(via->getPosition());
        }
        mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
        sgl.dismiss();
    }
    mVia = via;
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_NetPoint::setPosition(const Point& position) noexcept
{
    if (position != mPosition) {
        mPosition = position;
        mGraphicsItem->setPos(mPosition.toPxQPointF());
        updateLines();
        mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_NetPoint::addToBoard()
{
    if (isAddedToBoard() || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isAttachedToPad()) {
        // check if netsignal is correct (would be a bug if not)
        if (mFootprintPad->getCompSigInstNetSignal() != &mNetSegment.getNetSignal()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mFootprintPad->registerNetPoint(*this); // can throw
    } else if (isAttachedToVia()) {
        // check if netsignal is correct (would be a bug if not)
        if (&mVia->getNetSegment() != &mNetSegment) {
            throw LogicError(__FILE__, __LINE__);
        }
        mVia->registerNetPoint(*this); // can throw
    }
    mHighlightChangedConnection = connect(&getNetSignalOfNetSegment(),
                                          &NetSignal::highlightedChanged,
                                          [this](){mGraphicsItem->update();});
    mErcMsgDeadNetPoint->setVisible(true);
    BI_Base::addToBoard(mGraphicsItem.data());
    mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
}

void BI_NetPoint::removeFromBoard()
{
    if ((!isAddedToBoard()) || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isAttachedToPad()) {
        // check if netsignal is correct (would be a bug if not)
        if (mFootprintPad->getCompSigInstNetSignal() != &mNetSegment.getNetSignal()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mFootprintPad->unregisterNetPoint(*this); // can throw
    } else if (isAttachedToVia()) {
        // check if netsignal is correct (would be a bug if not)
        if (&mVia->getNetSegment() != &mNetSegment) {
            throw LogicError(__FILE__, __LINE__);
        }
        mVia->unregisterNetPoint(*this); // can throw
    }
    disconnect(mHighlightChangedConnection);
    mErcMsgDeadNetPoint->setVisible(false);
    BI_Base::removeFromBoard(mGraphicsItem.data());
    mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
}

void BI_NetPoint::registerNetLine(BI_NetLine& netline)
{
    if ((!isAddedToBoard()) || (mRegisteredLines.contains(&netline))
        || (netline.getBoard() != mBoard))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredLines.append(&netline);
    netline.updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mRegisteredLines.isEmpty());
}

void BI_NetPoint::unregisterNetLine(BI_NetLine& netline)
{
    if ((!isAddedToBoard()) || (!mRegisteredLines.contains(&netline))) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredLines.removeOne(&netline);
    netline.updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mRegisteredLines.isEmpty());
}

void BI_NetPoint::updateLines() const noexcept
{
    foreach (BI_NetLine* line, mRegisteredLines) {
        line->updateLine();
    }
}

void BI_NetPoint::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    root.appendChild("layer", SExpression::createToken(mLayer->getName()), false);
    if (isAttachedToPad()) {
        root.appendChild("dev", mFootprintPad->getFootprint().getComponentInstanceUuid(), true);
        root.appendChild("pad", mFootprintPad->getLibPadUuid(), false);
    } else if (isAttachedToVia()) {
        root.appendChild("via", mVia->getUuid(), true);
    } else {
        root.appendChild(mPosition.serializeToDomElement("pos"), true);
    }
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_NetPoint::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->shape().translated(mPosition.toPxQPointF());
}

bool BI_NetPoint::isSelectable() const noexcept
{
    return mGraphicsItem->isSelectable();
}

void BI_NetPoint::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool BI_NetPoint::checkAttributesValidity() const noexcept
{
    if (isAttachedToPad() && (&mNetSegment.getNetSignal() != mFootprintPad->getCompSigInstNetSignal())) return false;
    if (isAttachedToVia() && (&mNetSegment != &mVia->getNetSegment())) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
