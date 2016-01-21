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
#include "deviceinstance.h"
#include "board.h"
#include "../project.h"
#include "../library/projectlibrary.h"
#include <librepcblibrary/elements.h>
#include "../erc/ercmsg.h"
#include <librepcbcommon/fileio/xmldomelement.h>
#include "../circuit/circuit.h"
#include "../circuit/componentinstance.h"
#include "items/bi_footprint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DeviceInstance::DeviceInstance(Board& board, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mBoard(board), mAddedToBoard(false), mCompInstance(nullptr),
    mLibDevice(nullptr), mLibFootprint(nullptr), mFootprint(nullptr)
{
    // get component instance
    Uuid compInstUuid = domElement.getAttribute<Uuid>("component_instance", true);
    mCompInstance = mBoard.getProject().getCircuit().getComponentInstanceByUuid(compInstUuid);
    if (!mCompInstance)
    {
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
    mFootprint = new BI_Footprint(*this, *domElement.getFirstChild("footprint", true));

    init();
}

DeviceInstance::DeviceInstance(Board& board, ComponentInstance& compInstance,
                               const Uuid& deviceUuid, const Uuid& footprintUuid,
                               const Point& position, const Angle& rotation) throw (Exception) :
    QObject(nullptr), mBoard(board), mAddedToBoard(false), mCompInstance(&compInstance),
    mLibDevice(nullptr), mLibFootprint(nullptr), mFootprint(nullptr), mPosition(position),
    mRotation(rotation), mIsMirrored(false)
{
    initDeviceAndPackageAndFootprint(deviceUuid, footprintUuid);

    // create footprint
    mFootprint = new BI_Footprint(*this);

    init();
}

void DeviceInstance::initDeviceAndPackageAndFootprint(const Uuid& deviceUuid,
                                                      const Uuid& footprintUuid) throw (Exception)
{
    // get device from library
    mLibDevice = mBoard.getProject().getLibrary().getDevice(deviceUuid);
    if (!mLibDevice)
    {
        throw RuntimeError(__FILE__, __LINE__, mCompInstance->getUuid().toStr(),
            QString(tr("No device with the UUID \"%1\" found in the project's library."))
            .arg(deviceUuid.toStr()));
    }
    // check if the device matches with the component
    if (mLibDevice->getComponentUuid() != mCompInstance->getLibComponent().getUuid())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The device \"%1\" does not match with the component"
            "instance \"%2\".")).arg(mLibDevice->getUuid().toStr(),
            mCompInstance->getUuid().toStr()));
    }
    // get package from library
    Uuid packageUuid = mLibDevice->getPackageUuid();
    mLibPackage = mBoard.getProject().getLibrary().getPackage(packageUuid);
    if (!mLibPackage)
    {
        throw RuntimeError(__FILE__, __LINE__, mCompInstance->getUuid().toStr(),
            QString(tr("No package with the UUID \"%1\" found in the project's library."))
            .arg(packageUuid.toStr()));
    }
    // get footprint from package
    mLibFootprint = mLibPackage->getFootprintByUuid(footprintUuid);
    if (!mLibFootprint)
    {
        throw RuntimeError(__FILE__, __LINE__, mCompInstance->getUuid().toStr(),
            QString(tr("The package \"%1\" does not have a footprint with the UUID \"%2\"."))
            .arg(packageUuid.toStr()).arg(footprintUuid.toStr()));
    }
}

void DeviceInstance::init() throw (Exception)
{
    // check pad-signal-map
    foreach (const Uuid& signalUuid, mLibDevice->getPadSignalMap())
    {
        if ((!signalUuid.isNull()) && (!mCompInstance->getSignalInstance(signalUuid)))
        {
            throw RuntimeError(__FILE__, __LINE__, signalUuid.toStr(),
                QString(tr("Unknown signal \"%1\" found in device \"%2\""))
                .arg(signalUuid.toStr(), mLibDevice->getUuid().toStr()));
        }
    }

    // emit the "attributesChanged" signal when the board has emited it
    connect(&mBoard, &Board::attributesChanged, this, &DeviceInstance::attributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

DeviceInstance::~DeviceInstance() noexcept
{
    Q_ASSERT(!mAddedToBoard);
    delete mFootprint;          mFootprint = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Project& DeviceInstance::getProject() const noexcept
{
    return mBoard.getProject();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void DeviceInstance::setPosition(const Point& pos) noexcept
{
    mPosition = pos;
    emit moved(mPosition);
}

void DeviceInstance::setRotation(const Angle& rot) noexcept
{
    mRotation = rot;
    emit rotated(mRotation);
}

void DeviceInstance::setIsMirrored(bool mirror) noexcept
{
    mIsMirrored = mirror;
    emit mirrored(mIsMirrored);
}

void DeviceInstance::addToBoard(GraphicsScene& scene) throw (Exception)
{
    if (mAddedToBoard) throw LogicError(__FILE__, __LINE__);
    mCompInstance->registerDevice(*this);
    mFootprint->addToBoard(scene);
    mAddedToBoard = true;
    updateErcMessages();
}

void DeviceInstance::removeFromBoard(GraphicsScene& scene) throw (Exception)
{
    if (!mAddedToBoard) throw LogicError(__FILE__, __LINE__);
    mCompInstance->unregisterDevice(*this);
    mFootprint->removeFromBoard(scene);
    mAddedToBoard = false;
    updateErcMessages();
}

XmlDomElement* DeviceInstance::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("device_instance"));
    root->setAttribute("component_instance", mCompInstance->getUuid());
    root->setAttribute("device", mLibDevice->getUuid());
    root->setAttribute("footprint", mLibFootprint->getUuid());
    root->appendChild(mFootprint->serializeToXmlDomElement());
    XmlDomElement* position = root->appendChild("position");
    position->setAttribute("x", mPosition.getX());
    position->setAttribute("y", mPosition.getY());
    position->setAttribute("rotation", mRotation);
    position->setAttribute("mirror", mIsMirrored);
    return root.take();
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool DeviceInstance::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                        bool passToParents, QString& value) const noexcept
{
    // no local attributes available

    if (((attrNS == QLatin1String("CMP")) || (attrNS.isEmpty())) && passToParents)
    {
        if (mCompInstance->getAttributeValue(attrNS, attrKey, false, value))
            return true;
    }

    if ((attrNS != QLatin1String("CMP")) && (passToParents))
        return mBoard.getAttributeValue(attrNS, attrKey, true, value);
    else
        return false;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool DeviceInstance::checkAttributesValidity() const noexcept
{
    if (mCompInstance == nullptr)            return false;
    if (mLibDevice == nullptr)                  return false;
    if (mLibPackage == nullptr)                    return false;
    return true;
}

void DeviceInstance::updateErcMessages() noexcept
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
