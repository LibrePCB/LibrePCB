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
#include "bi_via.h"
#include "bi_netpoint.h"
#include "bi_netline.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/boardlayer.h>
#include <librepcb/common/scopeguardlist.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Via::BI_Via(Board& board, const BI_Via& other) :
    BI_Base(board), mUuid(Uuid::createRandom()), mPosition(other.mPosition),
    mShape(other.mShape), mSize(other.mSize), mDrillDiameter(other.mDrillDiameter),
    mNetSignal(other.mNetSignal)
{
    init();
}

BI_Via::BI_Via(Board& board, const DomElement& domElement) :
    BI_Base(board), mNetSignal(nullptr)
{
    // read attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mPosition.setX(domElement.getAttribute<Length>("x", true));
    mPosition.setY(domElement.getAttribute<Length>("y", true));
    QString shapeStr = domElement.getAttribute<QString>("shape", true);
    if (shapeStr == "round") {
        mShape = Shape::Round;
    } else if (shapeStr == "square") {
        mShape = Shape::Square;
    } else if (shapeStr == "octagon") {
        mShape = Shape::Octagon;
    } else {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid via shape: \"%1\"")).arg(shapeStr));
    }
    mSize = domElement.getAttribute<Length>("size", true);
    mDrillDiameter = domElement.getAttribute<Length>("drill", true);
    Uuid netSignalUuid = domElement.getAttribute<Uuid>("netsignal", false);
    if (!netSignalUuid.isNull()) {
        mNetSignal = mBoard.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
        if(!mNetSignal) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid.toStr()));
        }
    }

    init();
}

BI_Via::BI_Via(Board& board, const Point& position, Shape shape, const Length& size,
               const Length& drillDiameter, NetSignal* netsignal) :
    BI_Base(board), mUuid(Uuid::createRandom()), mPosition(position), mShape(shape),
    mSize(size), mDrillDiameter(drillDiameter), mNetSignal(netsignal)
{
    init();
}

void BI_Via::init()
{
    // create the graphics item
    mGraphicsItem.reset(new BGI_Via(*this));
    mGraphicsItem->setPos(mPosition.toPxQPointF());

    // connect to the "attributes changed" signal of the board
    connect(&mBoard, &Board::attributesChanged,
            this, &BI_Via::boardAttributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_Via::~BI_Via() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool BI_Via::isOnLayer(int layerId) const noexcept
{
    return BoardLayer::isCopperLayer(layerId);
}

QPainterPath BI_Via::toQPainterPathPx(const Length& clearance, bool hole) const noexcept
{
    QPainterPath p;
    p.setFillRule(Qt::OddEvenFill); // important to subtract the hole!
    switch (mShape)
    {
        case Shape::Round: {
            qreal dia = (mSize + clearance*2).toPx();
            p.addEllipse(-dia/2, -dia/2, dia, dia);
            break;
        }
        case Shape::Square: {
            qreal size = (mSize + clearance*2).toPx();
            p.addRect(-size/2, -size/2, size, size);
            break;
        }
        case Shape::Octagon: {
            qreal r = (mSize/2 + clearance).toPx();
            qreal a = r * (2 - qSqrt(2));
            QPolygonF octagon;
            octagon.append(QPointF(r, r-a));
            octagon.append(QPointF(r-a, r));
            octagon.append(QPointF(a-r, r));
            octagon.append(QPointF(-r, r-a));
            octagon.append(QPointF(-r, a-r));
            octagon.append(QPointF(a-r, -r));
            octagon.append(QPointF(r-a, -r));
            octagon.append(QPointF(r, a-r));
            p.addPolygon(octagon);
            break;
        }
        default: Q_ASSERT(false); break;
    }
    if (hole) {
        // remove hole
        p.addEllipse(QPointF(0, 0), mDrillDiameter.toPx()/2, mDrillDiameter.toPx()/2);
    }
    return p;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void BI_Via::setNetSignal(NetSignal* netsignal)
{
    if (netsignal == mNetSignal) {
        return;
    }
    if ((isUsed()) || (netsignal && (netsignal->getCircuit() != getCircuit()))) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
        ScopeGuardList sgl;
        if (mNetSignal) {
            mNetSignal->unregisterBoardVia(*this); // can throw
            sgl.add([&](){mNetSignal->registerBoardVia(*this);});
        }
        if (netsignal) {
            netsignal->registerBoardVia(*this); // can throw
            sgl.add([&](){netsignal->unregisterBoardVia(*this);});
        }
        sgl.dismiss();
    }
    mNetSignal = netsignal;
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::setPosition(const Point& position) noexcept
{
    if (position != mPosition) {
        mPosition = position;
        mGraphicsItem->setPos(mPosition.toPxQPointF());
        updateNetPoints();
    }
}

void BI_Via::setShape(Shape shape) noexcept
{
    if (shape != mShape) {
        mShape = shape;
        mGraphicsItem->updateCacheAndRepaint();
    }
}

void BI_Via::setSize(const Length& size) noexcept
{
    if (size != mSize) {
        mSize = size;
        mGraphicsItem->updateCacheAndRepaint();
    }
}

void BI_Via::setDrillDiameter(const Length& diameter) noexcept
{
    if (diameter != mDrillDiameter) {
        mDrillDiameter = diameter;
        mGraphicsItem->updateCacheAndRepaint();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_Via::addToBoard(GraphicsScene& scene)
{
    if (isAddedToBoard() || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (mNetSignal) {
        mNetSignal->registerBoardVia(*this); // can throw
        mHighlightChangedConnection = connect(mNetSignal, &NetSignal::highlightedChanged,
                                              [this](){mGraphicsItem->update();});
    }
    BI_Base::addToBoard(scene, *mGraphicsItem);
}

void BI_Via::removeFromBoard(GraphicsScene& scene)
{
    if ((!isAddedToBoard()) || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (mNetSignal) {
        mNetSignal->unregisterBoardVia(*this); // can throw
        disconnect(mHighlightChangedConnection);
    }
    BI_Base::removeFromBoard(scene, *mGraphicsItem);
}

void BI_Via::registerNetPoint(BI_NetPoint& netpoint)
{
    if ((!isAddedToBoard()) || (mRegisteredNetPoints.contains(netpoint.getLayer().getId()))
        || (netpoint.getBoard() != mBoard) || (&netpoint.getNetSignal() != mNetSignal))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoints.insert(netpoint.getLayer().getId(), &netpoint);
    netpoint.updateLines();
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::unregisterNetPoint(BI_NetPoint& netpoint)
{
    if ((!isAddedToBoard()) || (getNetPointOfLayer(netpoint.getLayer().getId()) != &netpoint)) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoints.remove(netpoint.getLayer().getId());
    netpoint.updateLines();
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::updateNetPoints() const noexcept
{
    foreach (BI_NetPoint* point, mRegisteredNetPoints) {
        point->setPosition(mPosition);
    }
}

void BI_Via::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("uuid", mUuid);
    root.setAttribute("x", mPosition.getX());
    root.setAttribute("y", mPosition.getY());
    switch (mShape)
    {
        case Shape::Round:      root.setAttribute<QString>("shape", "round"); break;
        case Shape::Square:     root.setAttribute<QString>("shape", "square"); break;
        case Shape::Octagon:    root.setAttribute<QString>("shape", "octagon"); break;
        default: throw LogicError(__FILE__, __LINE__);
    }
    root.setAttribute("size", mSize);
    root.setAttribute("drill", mDrillDiameter);
    root.setAttribute("netsignal", mNetSignal ? mNetSignal->getUuid() : Uuid());
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_Via::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->shape().translated(mPosition.toPxQPointF());
}

bool BI_Via::isSelectable() const noexcept
{
    return mGraphicsItem->isSelectable();
}

void BI_Via::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BI_Via::boardAttributesChanged()
{
    mGraphicsItem->updateCacheAndRepaint();
}

bool BI_Via::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                             return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
