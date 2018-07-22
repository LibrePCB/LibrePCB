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
#include "si_netline.h"
#include "../schematic.h"
#include "si_netpoint.h"
#include "si_netsegment.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include "si_symbol.h"
#include "si_symbolpin.h"
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

SI_NetLine::SI_NetLine(SI_NetSegment& segment, const SExpression& node) :
    SI_Base(segment.getSchematic()), mPosition(), mUuid(),
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

SI_NetLine::SI_NetLine(SI_NetPoint& startPoint, SI_NetPoint& endPoint,
                       const Length& width) :
    SI_Base(startPoint.getSchematic()), mPosition(), mUuid(Uuid::createRandom()),
    mStartPoint(&startPoint), mEndPoint(&endPoint), mWidth(width)
{
    init();
}

void SI_NetLine::init()
{
    if(mWidth < 0) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net line width: \"%1\"")).arg(mWidth.toMmString()));
    }

    // check if both netpoints are in the same net segment
    if (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment()) {
        throw LogicError(__FILE__, __LINE__,
            tr("SI_NetLine: endpoints netsegment mismatch."));
    }

    // check if both netpoints are different
    if (mStartPoint == mEndPoint) {
        throw LogicError(__FILE__, __LINE__,
            tr("SI_NetLine: both endpoints are the same."));
    }

    mGraphicsItem.reset(new SGI_NetLine(*this));
    updateLine();

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SI_NetLine::~SI_NetLine() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

SI_NetSegment& SI_NetLine::getNetSegment() const noexcept
{
    Q_ASSERT(mStartPoint && mEndPoint);
    Q_ASSERT(&mStartPoint->getNetSegment() == &mEndPoint->getNetSegment());
    return mStartPoint->getNetSegment();
}

SI_NetPoint* SI_NetLine::getOtherPoint(const SI_NetPoint& firstPoint) const noexcept
{
    if (&firstPoint == mStartPoint) {
        return mEndPoint;
    } else if (&firstPoint == mEndPoint) {
        return mStartPoint;
    } else {
        return nullptr;
    }
}

NetSignal& SI_NetLine::getNetSignalOfNetSegment() const noexcept
{
    return getNetSegment().getNetSignal();
}

bool SI_NetLine::isAttachedToSymbol() const noexcept
{
    return (mStartPoint->isAttachedToPin() || mEndPoint->isAttachedToPin());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SI_NetLine::setWidth(const Length& width) noexcept
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

void SI_NetLine::addToSchematic()
{
    if (isAddedToSchematic() || (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment())) {
        throw LogicError(__FILE__, __LINE__);
    }

    mStartPoint->registerNetLine(*this); // can throw
    auto sg = scopeGuard([&](){mStartPoint->unregisterNetLine(*this);});
    mEndPoint->registerNetLine(*this); // can throw

    mHighlightChangedConnection = connect(&getNetSignalOfNetSegment(),
                                          &NetSignal::highlightedChanged,
                                          [this](){mGraphicsItem->update();});
    SI_Base::addToSchematic(mGraphicsItem.data());
    sg.dismiss();
}

void SI_NetLine::removeFromSchematic()
{
    if ((!isAddedToSchematic()) || (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment())) {
        throw LogicError(__FILE__, __LINE__);
    }

    mEndPoint->unregisterNetLine(*this); // can throw
    auto sg = scopeGuard([&](){mEndPoint->registerNetLine(*this);});
    mStartPoint->unregisterNetLine(*this); // can throw

    disconnect(mHighlightChangedConnection);
    SI_Base::removeFromSchematic(mGraphicsItem.data());
    sg.dismiss();
}

void SI_NetLine::updateLine() noexcept
{
    mPosition = (mStartPoint->getPosition() + mEndPoint->getPosition()) / 2;
    mGraphicsItem->updateCacheAndRepaint();
}

void SI_NetLine::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    root.appendChild("width", mWidth, false);
    root.appendChild("p1", mStartPoint->getUuid(), true);
    root.appendChild("p2", mEndPoint->getUuid(), true);
}

/*****************************************************************************************
 *  Inherited from SI_Base
 ****************************************************************************************/

QPainterPath SI_NetLine::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->shape();
}

void SI_NetLine::setSelected(bool selected) noexcept
{
    SI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SI_NetLine::checkAttributesValidity() const noexcept
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
