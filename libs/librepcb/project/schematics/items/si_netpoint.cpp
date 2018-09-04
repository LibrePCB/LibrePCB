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
#include "si_netsegment.h"
#include "../../circuit/netsignal.h"
#include "../../erc/ercmsg.h"

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
    mPosition(0, 0)
{
    if (node.tryGetChildByPath("position")) {
        mPosition = Point(node.getChildByPath("position"));
    } else {
        // backward compatibility, remove this some time!
        mPosition = Point(node.getChildByPath("pos"));
    }

    init();
}

SI_NetPoint::SI_NetPoint(SI_NetSegment& segment, const Point& position) :
    SI_Base(segment.getSchematic()), mNetSegment(segment), mUuid(Uuid::createRandom()),
    mPosition(position)
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
        .arg(*mSchematic.getName()).arg(mUuid.toStr())));
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
    return (mRegisteredNetLines.count() > 2);
}

bool SI_NetPoint::isOpenLineEnd() const noexcept
{
    return (mRegisteredNetLines.count() <= 1);
}

NetSignal& SI_NetPoint::getNetSignalOfNetSegment() const noexcept
{
    return mNetSegment.getNetSignal();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SI_NetPoint::setPosition(const Point& position) noexcept
{
    if (position != mPosition) {
        mPosition = position;
        mGraphicsItem->setPos(mPosition.toPxQPointF());
        foreach (SI_NetLine* line, mRegisteredNetLines) {
            line->updateLine();
        }
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
    disconnect(mHighlightChangedConnection);
    mErcMsgDeadNetPoint->setVisible(false);
    SI_Base::removeFromSchematic(mGraphicsItem.data());
}

void SI_NetPoint::registerNetLine(SI_NetLine& netline)
{
    if ((!isAddedToSchematic()) || (mRegisteredNetLines.contains(&netline))
        || (&netline.getNetSegment() != &mNetSegment))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetLines.insert(&netline);
    netline.updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mRegisteredNetLines.isEmpty());
}

void SI_NetPoint::unregisterNetLine(SI_NetLine& netline)
{
    if ((!isAddedToSchematic()) || (!mRegisteredNetLines.contains(&netline))) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetLines.remove(&netline);
    netline.updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mRegisteredNetLines.isEmpty());
}

void SI_NetPoint::serialize(SExpression& root) const
{
    root.appendChild(mUuid);
    root.appendChild(mPosition.serializeToDomElement("position"), false);
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
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
