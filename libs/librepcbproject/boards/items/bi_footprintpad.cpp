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
#include "../board.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../settings/projectsettings.h"
#include <librepcbcommon/graphics/graphicsscene.h>
#include "bi_device.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_FootprintPad::BI_FootprintPad(BI_Footprint& footprint, const Uuid& padUuid) :
    BI_Base(footprint.getBoard()), mFootprint(footprint), mFootprintPad(nullptr)
   /*mComponentSignal(nullptr), mComponentSignalInstance(nullptr), mRegisteredNetPoint(nullptr),*/
{
    // read attributes
    mFootprintPad = mFootprint.getLibFootprint().getPadByUuid(padUuid);
    if (!mFootprintPad) {
        throw RuntimeError(__FILE__, __LINE__, padUuid.toStr(),
            QString(tr("Invalid footprint pad UUID: \"%1\"")).arg(padUuid.toStr()));
    }

    mGraphicsItem.reset(new BGI_FootprintPad(*this));

    // connect to the "attributes changed" signal of the footprint
    connect(&mFootprint, &BI_Footprint::attributesChanged,
            this, &BI_FootprintPad::footprintAttributesChanged);

    updatePosition();
}

BI_FootprintPad::~BI_FootprintPad()
{
    //Q_ASSERT(mRegisteredNetPoint == nullptr);
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const Uuid& BI_FootprintPad::getLibPadUuid() const noexcept
{
    return mFootprintPad->getUuid();
}

/*QString BI_FootprintPad::getDisplayText(bool returnCmpSignalNameIfEmpty,
                                     bool returnPinNameIfEmpty) const noexcept
{

}*/

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_FootprintPad::updatePosition() noexcept
{
    mPosition = mFootprint.mapToScene(mFootprintPad->getPosition());
    mRotation = mFootprint.getRotation() + mFootprintPad->getRotation();
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    updateGraphicsItemTransform();
    mGraphicsItem->updateCacheAndRepaint();
    //if (mRegisteredNetPoint)
    //    mRegisteredNetPoint->setPosition(mPosition);
}

/*void BI_FootprintPad::registerNetPoint(SI_NetPoint& netpoint)
{
    Q_ASSERT(mRegisteredNetPoint == nullptr);
    mRegisteredNetPoint = &netpoint;
    updateErcMessages();
}

void BI_FootprintPad::unregisterNetPoint(SI_NetPoint& netpoint)
{
    Q_UNUSED(netpoint); // to avoid compiler warning in release mode
    Q_ASSERT(mRegisteredNetPoint == &netpoint);
    mRegisteredNetPoint = nullptr;
    updateErcMessages();
}*/

void BI_FootprintPad::addToBoard(GraphicsScene& scene) throw (Exception)
{
    if (isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    //Q_ASSERT(mRegisteredNetPoint == nullptr);
    //mComponentSignalInstance->registerSymbolPin(*this);
    BI_Base::addToBoard(scene, *mGraphicsItem);
}

void BI_FootprintPad::removeFromBoard(GraphicsScene& scene) throw (Exception)
{
    if (!isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    //Q_ASSERT(mRegisteredNetPoint == nullptr);
    //mComponentSignalInstance->unregisterSymbolPin(*this);
    BI_Base::removeFromBoard(scene, *mGraphicsItem);
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
