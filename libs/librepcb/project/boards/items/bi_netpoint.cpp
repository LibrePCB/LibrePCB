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

BI_NetPoint::BI_NetPoint(Board& board, const BI_NetPoint& other, BI_FootprintPad* pad,
                         BI_Via* via) :
    BI_Base(board), mUuid(Uuid::createRandom()), mPosition(other.mPosition),
    mLayer(nullptr), mNetSignal(other.mNetSignal), mFootprintPad(pad), mVia(via)
{
    mLayer = mBoard.getLayerStack().getLayer(other.getLayer().getName());

    if (((other.getFootprintPad() == nullptr) != (mFootprintPad == nullptr))
        || ((other.getVia() == nullptr) != (mVia == nullptr)) || (!mLayer))
    {
        throw LogicError(__FILE__, __LINE__);
    }

    init();
}

BI_NetPoint::BI_NetPoint(Board& board, const DomElement& domElement) :
    BI_Base(board), mLayer(nullptr), mNetSignal(nullptr), mFootprintPad(nullptr),
    mVia(nullptr)
{
    // read attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);

    QString layerName = domElement.getAttribute<QString>("layer", true);
    mLayer = mBoard.getLayerStack().getLayer(layerName);
    if (!mLayer) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid board layer: \"%1\""))
            .arg(layerName));
    }

    Uuid netSignalUuid = domElement.getAttribute<Uuid>("netsignal", true);
    mNetSignal = mBoard.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if(!mNetSignal) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid.toStr()));
    }

    QString attachedTo = domElement.getAttribute<QString>("attached_to", true);
    if (attachedTo == "via") {
        Uuid viaUuid = domElement.getAttribute<Uuid>("via", true);
        mVia = mBoard.getViaByUuid(viaUuid);
        if (!mVia) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid via UUID: \"%1\"")).arg(viaUuid.toStr()));
        }
        mPosition = mVia->getPosition();
    } else if (attachedTo == "pad") {
        Uuid componentUuid = domElement.getAttribute<Uuid>("component", true);
        BI_Device* device = mBoard.getDeviceInstanceByComponentUuid(componentUuid);
        if (!device) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid component UUID: \"%1\"")).arg(componentUuid.toStr()));
        }
        Uuid padUuid = domElement.getAttribute<Uuid>("pad", true);
        mFootprintPad = device->getFootprint().getPad(padUuid);
        if (!mFootprintPad) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid footprint pad UUID: \"%1\"")).arg(padUuid.toStr()));
        }
        mPosition = mFootprintPad->getPosition();
    } else if (attachedTo == "none") {
        mPosition.setX(domElement.getAttribute<Length>("x", true));
        mPosition.setY(domElement.getAttribute<Length>("y", true));
    } else {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid 'attached_to' attribute: \"%1\"")).arg(attachedTo));
    }

    init();
}

BI_NetPoint::BI_NetPoint(Board& board, GraphicsLayer& layer, NetSignal& netsignal,
                         const Point& position) :
    BI_Base(board), mUuid(Uuid::createRandom()), mPosition(position), mLayer(&layer),
    mNetSignal(&netsignal), mFootprintPad(nullptr), mVia(nullptr)
{
    init();
}

BI_NetPoint::BI_NetPoint(Board& board, GraphicsLayer& layer, NetSignal& netsignal,
                         BI_FootprintPad& pad) :
    BI_Base(board), mUuid(Uuid::createRandom()), mPosition(pad.getPosition()),
    mLayer(&layer), mNetSignal(&netsignal), mFootprintPad(&pad), mVia(nullptr)
{
    init();
}

BI_NetPoint::BI_NetPoint(Board& board, GraphicsLayer& layer, NetSignal& netsignal,
                         BI_Via& via) :
    BI_Base(board), mUuid(Uuid::createRandom()), mPosition(via.getPosition()),
    mLayer(&layer), mNetSignal(&netsignal), mFootprintPad(nullptr), mVia(&via)
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
        if (mFootprintPad->getLibPad().getTechnology() == library::FootprintPad::Technology_t::SMT) {
            if (mLayer->getName() != mFootprintPad->getLayerName()) {
                throw RuntimeError(__FILE__, __LINE__,
                    QString(tr("The layer of netpoint \"%1\" is invalid (%2)."))
                    .arg(mUuid.toStr()).arg(mLayer->getName()));
            }
        }
    }

    // create the graphics item
    mGraphicsItem.reset(new BGI_NetPoint(*this));
    mGraphicsItem->setPos(mPosition.toPxQPointF());

    // create ERC messages
    mErcMsgDeadNetPoint.reset(new ErcMsg(mBoard.getProject(), *this,
        mUuid.toStr(), "Dead", ErcMsg::ErcMsgType_t::BoardError,
        QString(tr("Dead net point in board page \"%1\": %2"))
        .arg(mBoard.getName()).arg(mUuid.toStr())));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_NetPoint::~BI_NetPoint() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Length BI_NetPoint::getMaxLineWidth() const noexcept
{
    Length w = 0;
    foreach (BI_NetLine* line, mRegisteredLines) {
        if (line->getWidth() > w) {
            w = line->getWidth();
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

void BI_NetPoint::setNetSignal(NetSignal& netsignal)
{
    if (&netsignal == mNetSignal) {
        return;
    }
    if ((isUsed()) || (netsignal.getCircuit() != getCircuit())) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
        if (isAttached()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mNetSignal->unregisterBoardNetPoint(*this); // can throw
        auto sg = scopeGuard([&](){mNetSignal->registerBoardNetPoint(*this);});
        netsignal.registerBoardNetPoint(*this); // can throw
        sg.dismiss();
    }
    mNetSignal = &netsignal;
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
            if (pad->getCompSigInstNetSignal() != mNetSignal) {
                throw LogicError(__FILE__, __LINE__);
            }
            pad->registerNetPoint(*this); // can throw
            sgl.add([&](){pad->unregisterNetPoint(*this);});
            setPosition(pad->getPosition());
        }
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
            if (via->getNetSignal() != mNetSignal) throw LogicError(__FILE__, __LINE__);
            via->registerNetPoint(*this); // can throw
            sgl.add([&](){via->unregisterNetPoint(*this);});
            setPosition(via->getPosition());
        }
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
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_NetPoint::addToBoard(GraphicsScene& scene)
{
    if (isAddedToBoard() || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    ScopeGuardList sgl;
    mNetSignal->registerBoardNetPoint(*this); // can throw
    sgl.add([&](){mNetSignal->unregisterBoardNetPoint(*this);});
    if (isAttachedToPad()) {
        // check if mNetSignal is correct (would be a bug if not)
        if (mFootprintPad->getCompSigInstNetSignal() != mNetSignal) {
            throw LogicError(__FILE__, __LINE__);
        }
        mFootprintPad->registerNetPoint(*this); // can throw
        sgl.add([&](){mFootprintPad->unregisterNetPoint(*this);});
    } else if (isAttachedToVia()) {
        // check if mNetSignal is correct (would be a bug if not)
        if (mNetSignal != mVia->getNetSignal()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mVia->registerNetPoint(*this); // can throw
        sgl.add([&](){mVia->unregisterNetPoint(*this);});
    }
    mHighlightChangedConnection = connect(mNetSignal, &NetSignal::highlightedChanged,
                                          [this](){mGraphicsItem->update();});
    mErcMsgDeadNetPoint->setVisible(true);
    BI_Base::addToBoard(scene, *mGraphicsItem);
    sgl.dismiss();
}

void BI_NetPoint::removeFromBoard(GraphicsScene& scene)
{
    if ((!isAddedToBoard()) || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    ScopeGuardList sgl;
    if (isAttachedToPad()) {
        // check if mNetSignal is correct (would be a bug if not)
        if (mFootprintPad->getCompSigInstNetSignal() != mNetSignal) {
            throw LogicError(__FILE__, __LINE__);
        }
        mFootprintPad->unregisterNetPoint(*this); // can throw
        sgl.add([&](){mFootprintPad->registerNetPoint(*this);});
    } else if (isAttachedToVia()) {
        // check if mNetSignal is correct (would be a bug if not)
        if (mNetSignal != mVia->getNetSignal()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mVia->unregisterNetPoint(*this); // can throw
        sgl.add([&](){mVia->registerNetPoint(*this);});
    }
    mNetSignal->unregisterBoardNetPoint(*this); // can throw
    sgl.add([&](){mNetSignal->registerBoardNetPoint(*this);});
    disconnect(mHighlightChangedConnection);
    mErcMsgDeadNetPoint->setVisible(false);
    BI_Base::removeFromBoard(scene, *mGraphicsItem);
    sgl.dismiss();
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

void BI_NetPoint::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("uuid", mUuid);
    root.setAttribute("layer", mLayer->getName());
    root.setAttribute("netsignal", mNetSignal->getUuid());
    if (isAttachedToPad()) {
        root.setAttribute("attached_to", QString("pad"));
        root.setAttribute("component", mFootprintPad->getFootprint().getComponentInstanceUuid());
        root.setAttribute("pad", mFootprintPad->getLibPadUuid());
    } else if (isAttachedToVia()) {
        root.setAttribute("attached_to", QString("via"));
        root.setAttribute("via", mVia->getUuid());
    } else {
        root.setAttribute("attached_to", QString("none"));
        root.setAttribute("x", mPosition.getX());
        root.setAttribute("y", mPosition.getY());
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
    if (mUuid.isNull())                             return false;
    if (mNetSignal == nullptr)                      return false;
    if (isAttachedToPad() && (mNetSignal != mFootprintPad->getCompSigInstNetSignal())) return false;
    if (isAttachedToVia() && (mNetSignal != mVia->getNetSignal())) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
