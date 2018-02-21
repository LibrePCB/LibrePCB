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
#include "bi_netsegment.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include <librepcb/common/graphics/graphicsscene.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Via::BI_Via(BI_NetSegment& netsegment, const BI_Via& other) :
    BI_Base(netsegment.getBoard()), mNetSegment(netsegment), mUuid(Uuid::createRandom()),
    mPosition(other.mPosition), mShape(other.mShape), mSize(other.mSize),
    mDrillDiameter(other.mDrillDiameter)
{
    init();
}

BI_Via::BI_Via(BI_NetSegment& netsegment, const SExpression& node) :
    BI_Base(netsegment.getBoard()), mNetSegment(netsegment)
{
    // read attributes
    mUuid = node.getChildByIndex(0).getValue<Uuid>(true);
    mPosition = Point(node.getChildByPath("pos"));
    QString shapeStr = node.getValueByPath<QString>("shape", true);
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
    mSize = node.getValueByPath<Length>("size", true);
    mDrillDiameter = node.getValueByPath<Length>("drill", true);

    init();
}

BI_Via::BI_Via(BI_NetSegment& netsegment, const Point& position, Shape shape, const Length& size,
               const Length& drillDiameter) :
    BI_Base(netsegment.getBoard()), mNetSegment(netsegment), mUuid(Uuid::createRandom()),
    mPosition(position), mShape(shape), mSize(size), mDrillDiameter(drillDiameter)
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

NetSignal& BI_Via::getNetSignalOfNetSegment() const noexcept
{
    return mNetSegment.getNetSignal();
}

bool BI_Via::isOnLayer(const QString& layerName) const noexcept
{
    return GraphicsLayer::isCopperLayer(layerName);
}

Path BI_Via::getOutline(const Length& expansion) const noexcept
{
    Length size = mSize + (expansion * 2);
    if (size > 0) {
        switch (mShape) {
            case Shape::Round:      return Path::circle(size);
            case Shape::Square:     return Path::centeredRect(size, size);
            case Shape::Octagon:    return Path::octagon(size, size);
            default:                Q_ASSERT(false); break;
        }
    }
    return Path();
}

Path BI_Via::getSceneOutline(const Length& expansion) const noexcept
{
    return getOutline(expansion).translated(mPosition);
}

QPainterPath BI_Via::toQPainterPathPx(const Length& expansion) const noexcept
{
    QPainterPath p = getOutline(expansion).toQPainterPathPx();
    p.setFillRule(Qt::OddEvenFill); // important to subtract the hole!
    p.addEllipse(QPointF(0, 0), mDrillDiameter.toPx()/2, mDrillDiameter.toPx()/2);
    return p;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

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

void BI_Via::addToBoard()
{
    if (isAddedToBoard() || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    mHighlightChangedConnection = connect(&getNetSignalOfNetSegment(),
                                          &NetSignal::highlightedChanged,
                                          [this](){mGraphicsItem->update();});
    BI_Base::addToBoard(mGraphicsItem.data());
}

void BI_Via::removeFromBoard()
{
    if ((!isAddedToBoard()) || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    disconnect(mHighlightChangedConnection);
    BI_Base::removeFromBoard(mGraphicsItem.data());
}

void BI_Via::registerNetPoint(BI_NetPoint& netpoint)
{
    if ((!isAddedToBoard()) || (mRegisteredNetPoints.contains(netpoint.getLayer().getName()))
        || (netpoint.getBoard() != mBoard) || (&netpoint.getNetSegment() != &mNetSegment))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoints.insert(netpoint.getLayer().getName(), &netpoint);
    netpoint.updateLines();
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::unregisterNetPoint(BI_NetPoint& netpoint)
{
    if ((!isAddedToBoard()) || (getNetPointOfLayer(netpoint.getLayer().getName()) != &netpoint)) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoints.remove(netpoint.getLayer().getName());
    netpoint.updateLines();
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::updateNetPoints() const noexcept
{
    foreach (BI_NetPoint* point, mRegisteredNetPoints) {
        point->setPosition(mPosition);
    }
}

void BI_Via::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendChild(mPosition.serializeToDomElement("pos"), true);
    root.appendTokenChild("size", mSize, false);
    root.appendTokenChild("drill", mDrillDiameter, false);
    switch (mShape) {
        case Shape::Round:      root.appendTokenChild<QString>("shape", "round", false); break;
        case Shape::Square:     root.appendTokenChild<QString>("shape", "square", false); break;
        case Shape::Octagon:    root.appendTokenChild<QString>("shape", "octagon", false); break;
        default: throw LogicError(__FILE__, __LINE__);
    }
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
