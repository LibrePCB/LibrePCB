/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "bi_footprintpad.h"
#include "bi_footprint.h"
#include <librepcblibrary/pkg/footprint.h>
#include <librepcblibrary/pkg/footprintpad.h>
#include <librepcblibrary/pkg/packagepad.h>
#include "../board.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../settings/projectsettings.h"
#include <librepcbcommon/graphics/graphicsscene.h>
#include "bi_device.h"
#include <librepcblibrary/dev/device.h>
#include "../../circuit/componentinstance.h"
#include <librepcblibrary/cmp/component.h>
#include "bi_netpoint.h"
#include "../../circuit/componentsignalinstance.h"
#include <librepcbcommon/boardlayer.h>
#include "../../circuit/netsignal.h"
#include <librepcblibrary/pkg/package.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_FootprintPad::BI_FootprintPad(BI_Footprint& footprint, const Uuid& padUuid) :
    BI_Base(footprint.getBoard()), mFootprint(footprint), mFootprintPad(nullptr),
    mPackagePad(nullptr), mComponentSignalInstance(nullptr)
{
    mFootprintPad = mFootprint.getLibFootprint().getPadByUuid(padUuid);
    if (!mFootprintPad) {
        throw RuntimeError(__FILE__, __LINE__, padUuid.toStr(),
            QString(tr("Invalid footprint pad UUID: \"%1\"")).arg(padUuid.toStr()));
    }
    mPackagePad = mFootprint.getDeviceInstance().getLibPackage().getPadByUuid(padUuid);
    if (!mPackagePad) {
        throw RuntimeError(__FILE__, __LINE__, padUuid.toStr(),
            QString(tr("Invalid package pad UUID: \"%1\"")).arg(padUuid.toStr()));
    }
    Uuid cmpSignalUuid = mFootprint.getDeviceInstance().getLibDevice().getSignalOfPad(padUuid);
    mComponentSignalInstance = mFootprint.getDeviceInstance().getComponentInstance().getSignalInstance(cmpSignalUuid);

    mGraphicsItem.reset(new BGI_FootprintPad(*this));
    updatePosition();

    // connect to the "attributes changed" signal of the footprint
    connect(&mFootprint, &BI_Footprint::attributesChanged,
            this, &BI_FootprintPad::footprintAttributesChanged);
}

BI_FootprintPad::~BI_FootprintPad()
{
    Q_ASSERT(!isUsed());
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const Uuid& BI_FootprintPad::getLibPadUuid() const noexcept
{
    return mFootprintPad->getUuid();
}

QString BI_FootprintPad::getDisplayText() const noexcept
{
    NetSignal* signal = getCompSigInstNetSignal();
    if (signal) {
        return QString("%1:\n%2").arg(mPackagePad->getName(), signal->getName());
    } else {
        return mPackagePad->getName();
    }
}

int BI_FootprintPad::getLayerId() const noexcept
{
    if (getIsMirrored())
        return BoardLayer::getMirroredLayerId(mFootprintPad->getLayerId());
    else
        return mFootprintPad->getLayerId();
}

bool BI_FootprintPad::isOnLayer(const BoardLayer& layer) const noexcept
{
    if (getIsMirrored()) {
        return mFootprintPad->isOnLayer(layer.getMirroredLayerId());
    } else {
        return mFootprintPad->isOnLayer(layer.getId());
    }
}

NetSignal* BI_FootprintPad::getCompSigInstNetSignal() const noexcept
{
    if (mComponentSignalInstance) {
        return mComponentSignalInstance->getNetSignal();
    } else {
        return nullptr;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_FootprintPad::addToBoard(GraphicsScene& scene) throw (Exception)
{
    if (isAddedToBoard() || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (mComponentSignalInstance) {
        mComponentSignalInstance->registerFootprintPad(*this); // can throw
    }
    BI_Base::addToBoard(scene, *mGraphicsItem);
}

void BI_FootprintPad::removeFromBoard(GraphicsScene& scene) throw (Exception)
{
    if ((!isAddedToBoard()) || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (mComponentSignalInstance) {
        mComponentSignalInstance->unregisterFootprintPad(*this); // can throw
    }
    BI_Base::removeFromBoard(scene, *mGraphicsItem);
}

void BI_FootprintPad::registerNetPoint(BI_NetPoint& netpoint) throw (Exception)
{
    if ((!isAddedToBoard()) || (!mComponentSignalInstance)
        || (netpoint.getBoard() != mBoard)
        || (mRegisteredNetPoints.contains(netpoint.getLayer().getId()))
        || (&netpoint.getNetSignal() != mComponentSignalInstance->getNetSignal())
        || (!netpoint.getLayer().isCopperLayer())
        || ((mFootprintPad->getTechnology() == library::FootprintPad::Technology_t::SMT)
            && (netpoint.getLayer().getId() != getLayerId())))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoints.insert(netpoint.getLayer().getId(), &netpoint);
    netpoint.updateLines();
}

void BI_FootprintPad::unregisterNetPoint(BI_NetPoint& netpoint) throw (Exception)
{
    if ((!isAddedToBoard()) || (!mComponentSignalInstance)
        || (getNetPointOfLayer(netpoint.getLayer().getId()) != &netpoint)
        || (&netpoint.getNetSignal() != mComponentSignalInstance->getNetSignal()))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetPoints.remove(netpoint.getLayer().getId());
    netpoint.updateLines();
}

void BI_FootprintPad::updatePosition() noexcept
{
    mPosition = mFootprint.mapToScene(mFootprintPad->getPosition());
    mRotation = mFootprint.getRotation() + mFootprintPad->getRotation();
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    updateGraphicsItemTransform();
    mGraphicsItem->updateCacheAndRepaint();
    foreach (BI_NetPoint* netpoint, mRegisteredNetPoints) {
        netpoint->setPosition(mPosition);
    }
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

bool BI_FootprintPad::getIsMirrored() const noexcept
{
    return mFootprint.getIsMirrored();
}

QPainterPath BI_FootprintPad::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

bool BI_FootprintPad::isSelectable() const noexcept
{
    return mFootprint.isSelectable() && mGraphicsItem->isSelectable();
}

void BI_FootprintPad::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BI_FootprintPad::footprintAttributesChanged()
{
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BI_FootprintPad::updateGraphicsItemTransform() noexcept
{
    QTransform t;
    if (mFootprint.getIsMirrored()) t.scale(qreal(-1), qreal(1));
    t.rotate(-mRotation.toDeg());
    mGraphicsItem->setTransform(t);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
