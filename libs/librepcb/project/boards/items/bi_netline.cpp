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
#include "bi_netline.h"
#include "../board.h"
#include "bi_netpoint.h"
#include "bi_netsegment.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include "bi_footprint.h"
#include "bi_footprintpad.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/scopeguard.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_NetLine::BI_NetLine(const BI_NetLine& other, BI_NetPoint& startPoint, BI_NetPoint& endPoint) :
    BI_Base(startPoint.getBoard()), mPosition(other.mPosition), mUuid(Uuid::createRandom()),
    mStartPoint(&startPoint), mEndPoint(&endPoint), mWidth(other.mWidth)
{
    init();
}

BI_NetLine::BI_NetLine(BI_NetSegment& segment, const SExpression& node) :
    BI_Base(segment.getBoard()), mPosition(), mUuid(),
    mStartPoint(nullptr), mEndPoint(nullptr), mWidth()
{
    mUuid = node.getChildByIndex(0).getValue<Uuid>();
    mWidth = node.getValueByPath<Length>("width");

    Uuid spUuid = node.getValueByPath<Uuid>("p1");
    mStartPoint = segment.getNetPointByUuid(spUuid);
    if(!mStartPoint) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(spUuid.toStr()));
    }

    Uuid epUuid = node.getValueByPath<Uuid>("p2");
    mEndPoint = segment.getNetPointByUuid(epUuid);
    if(!mEndPoint) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(epUuid.toStr()));
    }

    init();
}

BI_NetLine::BI_NetLine(BI_NetPoint& startPoint, BI_NetPoint& endPoint, const Length& width) :
    BI_Base(startPoint.getBoard()), mPosition(), mUuid(Uuid::createRandom()),
    mStartPoint(&startPoint), mEndPoint(&endPoint), mWidth(width)
{
    init();
}

void BI_NetLine::init()
{
    if(mWidth < 0) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid trace width: \"%1\"")).arg(mWidth.toMmString()));
    }

    // check if both netpoints are in the same net segment
    if (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment()) {
        throw LogicError(__FILE__, __LINE__,
            tr("BI_NetLine: endpoints netsegment mismatch."));
    }

    // check if both netpoints have the same layer
    if (&mStartPoint->getLayer() != &mEndPoint->getLayer()) {
        throw LogicError(__FILE__, __LINE__,
            tr("BI_NetLine: endpoints layer mismatch."));
    }

    // check if both netpoints are different
    if (mStartPoint == mEndPoint) {
        throw LogicError(__FILE__, __LINE__,
            tr("BI_NetLine: both endpoints are the same."));
    }

    mGraphicsItem.reset(new BGI_NetLine(*this));
    updateLine();

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_NetLine::~BI_NetLine() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

BI_NetSegment& BI_NetLine::getNetSegment() const noexcept
{
    Q_ASSERT(mStartPoint && mEndPoint);
    Q_ASSERT(&mStartPoint->getNetSegment() == &mEndPoint->getNetSegment());
    return mStartPoint->getNetSegment();
}

GraphicsLayer& BI_NetLine::getLayer() const noexcept
{
    Q_ASSERT(&mStartPoint->getLayer() == &mEndPoint->getLayer());
    return mStartPoint->getLayer();
}

BI_NetPoint* BI_NetLine::getOtherPoint(const BI_NetPoint& firstPoint) const noexcept
{
    if (&firstPoint == mStartPoint) {
        return mEndPoint;
    } else if (&firstPoint == mEndPoint) {
        return mStartPoint;
    } else {
        return nullptr;
    }
}

NetSignal& BI_NetLine::getNetSignalOfNetSegment() const noexcept
{
    return getNetSegment().getNetSignal();
}

bool BI_NetLine::isAttached() const noexcept
{
    return (mStartPoint->isAttached() || mEndPoint->isAttached());
}

bool BI_NetLine::isAttachedToVia() const noexcept
{
    return (mStartPoint->isAttachedToVia() || mEndPoint->isAttachedToVia());
}

bool BI_NetLine::isAttachedToFootprint() const noexcept
{
    return (mStartPoint->isAttachedToPad() || mEndPoint->isAttachedToPad());
}

Path BI_NetLine::getSceneOutline(const Length& expansion) const noexcept
{
    Length width = mWidth + (expansion * 2);
    if (width > 0) {
        return Path::obround(mStartPoint->getPosition(), mEndPoint->getPosition(), width);
    } else {
        return Path();
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void BI_NetLine::setWidth(const Length& width) noexcept
{
    Q_ASSERT(width >= 0);
    if ((width != mWidth) && (width >= 0)) {
        mWidth = width;
        mGraphicsItem->updateCacheAndRepaint();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_NetLine::addToBoard()
{
    if (isAddedToBoard()
        || (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment())
        || (&mStartPoint->getLayer() != &mEndPoint->getLayer()))
    {
        throw LogicError(__FILE__, __LINE__);
    }

    mStartPoint->registerNetLine(*this); // can throw
    auto sg = scopeGuard([&](){mStartPoint->unregisterNetLine(*this);});
    mEndPoint->registerNetLine(*this); // can throw

    mHighlightChangedConnection = connect(&getNetSignalOfNetSegment(),
                                              &NetSignal::highlightedChanged,
                                              [this](){mGraphicsItem->update();});
    BI_Base::addToBoard(mGraphicsItem.data());
    sg.dismiss();
}

void BI_NetLine::removeFromBoard()
{
    if ((!isAddedToBoard())
        || (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment())
        || (&mStartPoint->getLayer() != &mEndPoint->getLayer()))
    {
        throw LogicError(__FILE__, __LINE__);
    }

    mStartPoint->unregisterNetLine(*this); // can throw
    auto sg = scopeGuard([&](){mEndPoint->registerNetLine(*this);});
    mEndPoint->unregisterNetLine(*this); // can throw

    disconnect(mHighlightChangedConnection);
    BI_Base::removeFromBoard(mGraphicsItem.data());
    sg.dismiss();
}

void BI_NetLine::updateLine() noexcept
{
    mPosition = (mStartPoint->getPosition() + mEndPoint->getPosition()) / 2;
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_NetLine::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    root.appendChild("width", mWidth, false);
    root.appendChild("p1", mStartPoint->getUuid(), true);
    root.appendChild("p2", mEndPoint->getUuid(), true);
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_NetLine::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->shape();
}

bool BI_NetLine::isSelectable() const noexcept
{
    return mGraphicsItem->isSelectable();
}

void BI_NetLine::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool BI_NetLine::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mStartPoint == nullptr) return false;
    if (mEndPoint == nullptr)   return false;
    if (mWidth < 0)             return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
