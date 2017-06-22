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
#include "bi_device.h"
#include "../board.h"
#include "../../project.h"
#include "../../library/projectlibrary.h"
#include <librepcb/library/elements.h>
#include "../../erc/ercmsg.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"
#include "bi_footprint.h"
#include <librepcb/common/scopeguard.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Device::BI_Device(Board& board, const BI_Device& other) throw (Exception) :
    BI_Base(board), mCompInstance(other.mCompInstance), mLibDevice(other.mLibDevice),
    mLibPackage(other.mLibPackage), mLibFootprint(other.mLibFootprint),
    mPosition(other.mPosition), mRotation(other.mRotation), mIsMirrored(other.mIsMirrored)
{
    mFootprint.reset(new BI_Footprint(*this, *other.mFootprint));

    init();
}

BI_Device::BI_Device(Board& board, const DomElement& domElement) throw (Exception) :
    BI_Base(board), mCompInstance(nullptr), mLibDevice(nullptr), mLibPackage(nullptr),
    mLibFootprint(nullptr)
{
    // get component instance
    Uuid compInstUuid = domElement.getAttribute<Uuid>("component", true);
    mCompInstance = mBoard.getProject().getCircuit().getComponentInstanceByUuid(compInstUuid);
    if (!mCompInstance) {
        throw RuntimeError(__FILE__, __LINE__, compInstUuid.toStr(),
            QString(tr("Could not find the component instance with UUID \"%1\"!"))
            .arg(compInstUuid.toStr()));
    }
    // get device and footprint uuid
    Uuid deviceUuid = domElement.getAttribute<Uuid>("device", true);
    Uuid footprintUuid = domElement.getAttribute<Uuid>("footprint", true);
    initDeviceAndPackageAndFootprint(deviceUuid, footprintUuid);

    // get position, rotation and mirrored
    mPosition.setX(domElement.getFirstChild("position", true)->getAttribute<Length>("x", true));
    mPosition.setY(domElement.getFirstChild("position", true)->getAttribute<Length>("y", true));
    mRotation = domElement.getFirstChild("position", true)->getAttribute<Angle>("rotation", true);
    mIsMirrored = domElement.getFirstChild("position", true)->getAttribute<bool>("mirror", true);

    // load footprint
    mFootprint.reset(new BI_Footprint(*this, *domElement.getFirstChild("footprint", true)));

    init();
}

BI_Device::BI_Device(Board& board, ComponentInstance& compInstance, const Uuid& deviceUuid,
        const Uuid& footprintUuid, const Point& position, const Angle& rotation, bool mirror) throw (Exception) :
    BI_Base(board), mCompInstance(&compInstance), mLibDevice(nullptr), mLibPackage(nullptr),
    mLibFootprint(nullptr), mPosition(position), mRotation(rotation), mIsMirrored(mirror)
{
    initDeviceAndPackageAndFootprint(deviceUuid, footprintUuid);

    // create footprint
    mFootprint.reset(new BI_Footprint(*this));

    init();
}

void BI_Device::initDeviceAndPackageAndFootprint(const Uuid& deviceUuid,
                                                 const Uuid& footprintUuid) throw (Exception)
{
    // get device from library
    mLibDevice = mBoard.getProject().getLibrary().getDevice(deviceUuid);
    if (!mLibDevice) {
        throw RuntimeError(__FILE__, __LINE__, mCompInstance->getUuid().toStr(),
            QString(tr("No device with the UUID \"%1\" found in the project's library."))
            .arg(deviceUuid.toStr()));
    }
    // check if the device matches with the component
    if (mLibDevice->getComponentUuid() != mCompInstance->getLibComponent().getUuid()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The device \"%1\" does not match with the component"
            "instance \"%2\".")).arg(mLibDevice->getUuid().toStr(),
            mCompInstance->getUuid().toStr()));
    }
    // get package from library
    Uuid packageUuid = mLibDevice->getPackageUuid();
    mLibPackage = mBoard.getProject().getLibrary().getPackage(packageUuid);
    if (!mLibPackage) {
        throw RuntimeError(__FILE__, __LINE__, mCompInstance->getUuid().toStr(),
            QString(tr("No package with the UUID \"%1\" found in the project's library."))
            .arg(packageUuid.toStr()));
    }
    // get footprint from package
    mLibFootprint = mLibPackage->getFootprintByUuid(footprintUuid);
    if (!mLibFootprint) {
        throw RuntimeError(__FILE__, __LINE__, mCompInstance->getUuid().toStr(),
            QString(tr("The package \"%1\" does not have a footprint with the UUID \"%2\"."))
            .arg(packageUuid.toStr()).arg(footprintUuid.toStr()));
    }
}

void BI_Device::init() throw (Exception)
{
    // check pad-signal-map
    foreach (const Uuid& signalUuid, mLibDevice->getPadSignalMap()) {
        if ((!signalUuid.isNull()) && (!mCompInstance->getSignalInstance(signalUuid))) {
            throw RuntimeError(__FILE__, __LINE__, signalUuid.toStr(),
                QString(tr("Unknown signal \"%1\" found in device \"%2\""))
                .arg(signalUuid.toStr(), mLibDevice->getUuid().toStr()));
        }
    }

    // emit the "attributesChanged" signal when the board has emited it
    connect(&mBoard, &Board::attributesChanged, this, &BI_Device::attributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_Device::~BI_Device() noexcept
{
    mFootprint.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const Uuid& BI_Device::getComponentInstanceUuid() const noexcept
{
    return mCompInstance->getUuid();
}

bool BI_Device::isUsed() const noexcept
{
    return mFootprint->isUsed();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_Device::setPosition(const Point& pos) noexcept
{
    if (pos != mPosition) {
        mPosition = pos;
        emit moved(mPosition);
    }
}

void BI_Device::setRotation(const Angle& rot) noexcept
{
    if (rot != mRotation) {
        mRotation = rot;
        emit rotated(mRotation);
    }
}

void BI_Device::setIsMirrored(bool mirror) throw (Exception)
{
    if (mirror != mIsMirrored) {
        if (isUsed()) {
            throw LogicError(__FILE__, __LINE__);
        }
        mIsMirrored = mirror;
        emit mirrored(mIsMirrored);
    }
}

void BI_Device::addToBoard(GraphicsScene& scene) throw (Exception)
{
    if (isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    mCompInstance->registerDevice(*this); // can throw
    auto sg = scopeGuard([&](){mCompInstance->unregisterDevice(*this);});
    mFootprint->addToBoard(scene); // can throw
    sg.dismiss();
    BI_Base::addToBoard();
    updateErcMessages();
}

void BI_Device::removeFromBoard(GraphicsScene& scene) throw (Exception)
{
    if (!isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    mFootprint->removeFromBoard(scene); // can throw
    auto sg = scopeGuard([&](){mFootprint->addToBoard(scene);});
    mCompInstance->unregisterDevice(*this); // can throw
    sg.dismiss();
    BI_Base::removeFromBoard();
    updateErcMessages();
}

void BI_Device::serialize(DomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("component", mCompInstance->getUuid());
    root.setAttribute("device", mLibDevice->getUuid());
    root.setAttribute("footprint", mLibFootprint->getUuid());
    root.appendChild(mFootprint->serializeToDomElement("footprint"));
    DomElement* position = root.appendChild("position");
    position->setAttribute("x", mPosition.getX());
    position->setAttribute("y", mPosition.getY());
    position->setAttribute("rotation", mRotation);
    position->setAttribute("mirror", mIsMirrored);
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool BI_Device::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                        bool passToParents, QString& value) const noexcept
{
    // no local attributes available

    if (((attrNS == QLatin1String("CMP")) || (attrNS.isEmpty())) && passToParents) {
        if (mCompInstance->getAttributeValue(attrNS, attrKey, false, value))
            return true;
    }

    if ((attrNS != QLatin1String("CMP")) && (passToParents))
        return mBoard.getAttributeValue(attrNS, attrKey, true, value);
    else
        return false;
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_Device::getGrabAreaScenePx() const noexcept
{
    return mFootprint->getGrabAreaScenePx();
}

bool BI_Device::isSelectable() const noexcept
{
    return mFootprint->isSelectable();
}

void BI_Device::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mFootprint->setSelected(selected);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool BI_Device::checkAttributesValidity() const noexcept
{
    if (mCompInstance == nullptr)            return false;
    if (mLibDevice == nullptr)               return false;
    if (mLibPackage == nullptr)              return false;
    return true;
}

void BI_Device::updateErcMessages() noexcept
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
