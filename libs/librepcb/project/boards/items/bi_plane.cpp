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
#include "bi_plane.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../graphicsitems/bgi_plane.h"
#include "../boardplanefragmentsbuilder.h"
#include <librepcb/common/scopeguard.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Plane::BI_Plane(Board& board, const BI_Plane& other) :
    BI_Base(board), mUuid(Uuid::createRandom()),
    mLayerName(other.mLayerName), mNetSignal(other.mNetSignal), mOutline(other.mOutline),
    mMinWidth(other.mMinWidth), mMinClearance(other.mMinClearance),
    mKeepOrphans(other.mKeepOrphans), mPriority(other.mPriority),
    mConnectStyle(other.mConnectStyle),
    //mThermalGapWidth(other.mThermalGapWidth), mThermalSpokeWidth(other.mThermalSpokeWidth),
    mFragments(other.mFragments) // also copy fragments to avoid the need for a rebuild
{
    init();
}

BI_Plane::BI_Plane(Board& board, const SExpression& node) :
    BI_Base(board)
{
    mUuid = node.getChildByIndex(0).getValue<Uuid>();
    mLayerName = node.getValueByPath<QString>("layer", true);
    Uuid netSignalUuid = node.getValueByPath<Uuid>("net");
    mNetSignal = mBoard.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if(!mNetSignal) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid.toStr()));
    }
    mMinWidth = node.getValueByPath<Length>("min_width");
    mMinClearance = node.getValueByPath<Length>("min_clearance");
    mKeepOrphans = node.getValueByPath<bool>("keep_orphans");
    mPriority = node.getValueByPath<int>("priority");
    mConnectStyle = node.getValueByPath<ConnectStyle>("connect_style");
    //mThermalGapWidth = node.getValueByPath<Length>("thermal_gap_width", true);
    //mThermalSpokeWidth = node.getValueByPath<Length>("thermal_spoke_width", true);
    mOutline = Path(node);
    init();
}

BI_Plane::BI_Plane(Board& board, const Uuid& uuid, const QString& layerName,
                   NetSignal& netsignal, const Path& outline) :
    BI_Base(board), mUuid(uuid), mLayerName(layerName), mNetSignal(&netsignal),
    mOutline(outline), mMinWidth(200000), mMinClearance(300000), mKeepOrphans(false),
    mPriority(0), mConnectStyle(ConnectStyle::Solid),
    //mThermalGapWidth(100000), mThermalSpokeWidth(100000),
    mFragments()
{
    init();
}

void BI_Plane::init()
{
    mGraphicsItem.reset(new BGI_Plane(*this));
    mGraphicsItem->setPos(getPosition().toPxQPointF());
    mGraphicsItem->setRotation(Angle::deg0().toDeg());

    // connect to the "attributes changed" signal of the board
    connect(&mBoard, &Board::attributesChanged, this, &BI_Plane::boardAttributesChanged);
}

BI_Plane::~BI_Plane() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void BI_Plane::setOutline(const Path& outline) noexcept
{
    if (outline != mOutline) {
        mOutline = outline;
        mGraphicsItem->updateCacheAndRepaint();
    }
}

void BI_Plane::setLayerName(const QString& layerName) noexcept
{
    if (layerName != mLayerName) {
        mLayerName = layerName;
        mGraphicsItem->updateCacheAndRepaint();
    }
}

void BI_Plane::setNetSignal(NetSignal& netsignal)
{
    if (&netsignal != mNetSignal) {
        if (netsignal.getCircuit() != getCircuit()) {
            throw LogicError(__FILE__, __LINE__);
        }
        if (isAddedToBoard()) {
            mNetSignal->unregisterBoardPlane(*this); // can throw
            auto sg = scopeGuard([&](){mNetSignal->registerBoardPlane(*this);});
            netsignal.registerBoardPlane(*this); // can throw
            sg.dismiss();
        }
        mNetSignal = &netsignal;
    }
}

void BI_Plane::setMinWidth(const Length& minWidth) noexcept
{
    if (minWidth != mMinWidth) {
        mMinWidth = minWidth;
    }
}

void BI_Plane::setMinClearance(const Length& minClearance) noexcept
{
    if (minClearance != mMinClearance) {
        mMinClearance = minClearance;
    }
}

void BI_Plane::setConnectStyle(BI_Plane::ConnectStyle style) noexcept
{
    if (style != mConnectStyle) {
        mConnectStyle = style;
    }
}

void BI_Plane::setPriority(int priority) noexcept
{
    if (priority != mPriority) {
        mPriority = priority;
    }
}

void BI_Plane::setKeepOrphans(bool keepOrphans) noexcept
{
    if (keepOrphans != mKeepOrphans) {
        mKeepOrphans = keepOrphans;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_Plane::addToBoard()
{
    if (isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    mNetSignal->registerBoardPlane(*this); // can throw
    BI_Base::addToBoard(mGraphicsItem.data());
    mGraphicsItem->updateCacheAndRepaint(); // TODO: remove this
    mBoard.scheduleAirWiresRebuild(mNetSignal);
}

void BI_Plane::removeFromBoard()
{
    if (!isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    mNetSignal->unregisterBoardPlane(*this); // can throw
    BI_Base::removeFromBoard(mGraphicsItem.data());
    mBoard.scheduleAirWiresRebuild(mNetSignal);
}

void BI_Plane::clear() noexcept
{
    mFragments.clear();
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Plane::rebuild() noexcept
{
    BoardPlaneFragmentsBuilder builder(*this);
    mFragments = builder.buildFragments();
    mGraphicsItem->updateCacheAndRepaint();
    mBoard.scheduleAirWiresRebuild(mNetSignal);
}

void BI_Plane::serialize(SExpression& root) const
{
    root.appendChild(mUuid);
    root.appendChild("layer", SExpression::createToken(mLayerName), false);
    root.appendChild("net", mNetSignal->getUuid(), true);
    root.appendChild("priority", mPriority, false);
    root.appendChild("min_width", mMinWidth, true);
    root.appendChild("min_clearance", mMinClearance, false);
    root.appendChild("keep_orphans", mKeepOrphans, false);
    root.appendChild("connect_style", mConnectStyle, true);
    //root.appendChild("thermal_gap_width", mThermalGapWidth, false);
    //root.appendChild("thermal_spoke_width", mThermalSpokeWidth, false);
    mOutline.serialize(root);
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_Plane::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

bool BI_Plane::isSelectable() const noexcept
{
    return mGraphicsItem->isSelectable();
}

void BI_Plane::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool BI_Plane::operator<(const BI_Plane& rhs) const noexcept
{
    // First sort by priority, then by uuid to get a really unique priority order over all
    // existing planes. This way we can ensure that even planes with the same priority
    // will always be filled in the same order. Random order would be dangerous!
    if (mPriority != rhs.mPriority) {
        return mPriority < rhs.mPriority;
    } else {
        return mUuid < rhs.mUuid;
    }
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BI_Plane::boardAttributesChanged()
{
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
