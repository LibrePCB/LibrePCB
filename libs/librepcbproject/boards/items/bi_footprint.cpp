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
#include "bi_footprint.h"
#include "bi_footprintpad.h"
#include "../board.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../library/projectlibrary.h"
#include <librepcblibrary/pkg/footprint.h>
#include <librepcblibrary/pkg/package.h>
#include <librepcblibrary/dev/device.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcbcommon/graphics/graphicsscene.h>
#include "../deviceinstance.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Footprint::BI_Footprint(DeviceInstance& device, const XmlDomElement& domElement) throw (Exception) :
    BI_Base(), mDeviceInstance(device), mGraphicsItem(nullptr)
{
    Q_UNUSED(domElement);
    init();
}

BI_Footprint::BI_Footprint(DeviceInstance& device) throw (Exception) :
    BI_Base(), mDeviceInstance(device), mGraphicsItem(nullptr)
{
    init();
}

void BI_Footprint::init() throw (Exception)
{
    mGraphicsItem = new BGI_Footprint(*this);
    mGraphicsItem->setPos(mDeviceInstance.getPosition().toPxQPointF());
    updateGraphicsItemTransform();

    const library::Device& libDev = mDeviceInstance.getLibDevice();
    foreach (const Uuid& padUuid, getLibFootprint().getPadUuids())
    {
        const library::FootprintPad* libPad = getLibFootprint().getPadByUuid(padUuid);
        Q_ASSERT(libPad); if (!libPad) continue;

        BI_FootprintPad* pad = new BI_FootprintPad(*this, libPad->getUuid());
        if (mPads.contains(libPad->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, libPad->getUuid().toStr(),
                QString(tr("The footprint pad UUID \"%1\" is defined multiple times."))
                .arg(libPad->getUuid().toStr()));
        }
        if (!libDev.getPadSignalMap().contains(libPad->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, libPad->getUuid().toStr(),
                QString(tr("Footprint pad \"%1\" not found in pad-signal-map of device \"%2\"."))
                .arg(libPad->getUuid().toStr(), libDev.getUuid().toStr()));
        }
        mPads.insert(libPad->getUuid(), pad);
    }
    if (mPads.count() != libDev.getPadSignalMap().count())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mPads.count()).arg(libDev.getPadSignalMap().count()),
            QString(tr("The pad count of the footprint \"%1\" does not match with "
            "the pad-signal-map of device \"%2\".")).arg(getLibFootprint().getUuid().toStr(),
            libDev.getUuid().toStr()));
    }

    // connect to the "attributes changed" signal of device instance
    connect(&mDeviceInstance, &DeviceInstance::attributesChanged,
            this, &BI_Footprint::deviceInstanceAttributesChanged);
    connect(&mDeviceInstance, &DeviceInstance::moved,
            this, &BI_Footprint::deviceInstanceMoved);
    connect(&mDeviceInstance, &DeviceInstance::rotated,
            this, &BI_Footprint::deviceInstanceRotated);
    connect(&mDeviceInstance, &DeviceInstance::mirrored,
            this, &BI_Footprint::deviceInstanceMirrored);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_Footprint::~BI_Footprint() noexcept
{
    qDeleteAll(mPads);              mPads.clear();
    delete mGraphicsItem;           mGraphicsItem = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Project& BI_Footprint::getProject() const noexcept
{
    return mDeviceInstance.getProject();
}

Board& BI_Footprint::getBoard() const noexcept
{
    return mDeviceInstance.getBoard();
}

const library::Footprint& BI_Footprint::getLibFootprint() const noexcept
{
    return mDeviceInstance.getLibFootprint();
}

const Angle& BI_Footprint::getRotation() const noexcept
{
    return mDeviceInstance.getRotation();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_Footprint::addToBoard(GraphicsScene& scene) throw (Exception)
{
    scene.addItem(*mGraphicsItem);
    foreach (BI_FootprintPad* pad, mPads)
        pad->addToBoard(scene);
}

void BI_Footprint::removeFromBoard(GraphicsScene& scene) throw (Exception)
{
    scene.removeItem(*mGraphicsItem);
    foreach (BI_FootprintPad* pad, mPads)
        pad->removeFromBoard(scene);
}

XmlDomElement* BI_Footprint::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("footprint"));
    //root->setAttribute("uuid", mUuid);
    //root->setAttribute("gen_comp_instance", mComponentInstance->getUuid());
    //root->setAttribute("symbol_item", mSymbVarItem->getUuid());
    return root.take();
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

Point BI_Footprint::mapToScene(const Point& relativePos) const noexcept
{
    if (mDeviceInstance.getIsMirrored()) {
        return (mDeviceInstance.getPosition() + relativePos)
                .rotated(mDeviceInstance.getRotation(), mDeviceInstance.getPosition())
                .mirrored(Qt::Horizontal, mDeviceInstance.getPosition());
    } else {
        return (mDeviceInstance.getPosition() + relativePos)
                .rotated(mDeviceInstance.getRotation(), mDeviceInstance.getPosition());
    }
}

bool BI_Footprint::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                     bool passToParents, QString& value) const noexcept
{
    // no local attributes available
    if (passToParents)
        return mDeviceInstance.getAttributeValue(attrNS, attrKey, true, value);
    else
        return false;
}

/*****************************************************************************************
 *  Inherited from SI_Base
 ****************************************************************************************/

const Point& BI_Footprint::getPosition() const noexcept
{
    return mDeviceInstance.getPosition();
}

bool BI_Footprint::getIsMirrored() const noexcept
{
    return mDeviceInstance.getIsMirrored();
}

QPainterPath BI_Footprint::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void BI_Footprint::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
    foreach (BI_FootprintPad* pad, mPads)
        pad->setSelected(selected);
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BI_Footprint::deviceInstanceAttributesChanged()
{
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Footprint::deviceInstanceMoved(const Point& pos)
{
    mGraphicsItem->setPos(pos.toPxQPointF());
    mGraphicsItem->updateCacheAndRepaint();
    foreach (BI_FootprintPad* pad, mPads)
        pad->updatePosition();
}

void BI_Footprint::deviceInstanceRotated(const Angle& rot)
{
    Q_UNUSED(rot);
    updateGraphicsItemTransform();
    mGraphicsItem->updateCacheAndRepaint();
    foreach (BI_FootprintPad* pad, mPads)
        pad->updatePosition();
}

void BI_Footprint::deviceInstanceMirrored(bool mirrored)
{
    Q_UNUSED(mirrored);
    updateGraphicsItemTransform();
    mGraphicsItem->updateCacheAndRepaint();
    foreach (BI_FootprintPad* pad, mPads)
        pad->updatePosition();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BI_Footprint::updateGraphicsItemTransform() noexcept
{
    QTransform t;
    if (mDeviceInstance.getIsMirrored()) t.scale(qreal(-1), qreal(1));
    t.rotate(-mDeviceInstance.getRotation().toDeg());
    mGraphicsItem->setTransform(t);
}

bool BI_Footprint::checkAttributesValidity() const noexcept
{
    //if (mUuid.isNull())                 return false;
    //if (mComponentInstance == nullptr)    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
