/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bi_device.h"

#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"
#include "../../erc/ercmsg.h"
#include "../../library/projectlibrary.h"
#include "../../project.h"
#include "../../settings/projectsettings.h"
#include "../board.h"
#include "bi_footprint.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/library/elements.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Device::BI_Device(Board& board, const BI_Device& other)
  : BI_Base(board),
    mCompInstance(other.mCompInstance),
    mLibDevice(other.mLibDevice),
    mLibPackage(other.mLibPackage),
    mLibFootprint(other.mLibFootprint),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mIsMirrored(other.mIsMirrored),
    mAttributes(other.mAttributes) {
  mFootprint.reset(new BI_Footprint(*this, *other.mFootprint));

  init();
}

BI_Device::BI_Device(Board& board, const SExpression& node)
  : BI_Base(board),
    mCompInstance(nullptr),
    mLibDevice(nullptr),
    mLibPackage(nullptr),
    mLibFootprint(nullptr),
    mAttributes() {
  // get component instance
  Uuid compInstUuid = node.getChildByIndex(0).getValue<Uuid>();
  mCompInstance =
      mBoard.getProject().getCircuit().getComponentInstanceByUuid(compInstUuid);
  if (!mCompInstance) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Could not find the component instance with UUID \"%1\"!")
            .arg(compInstUuid.toStr()));
  }
  // get device and footprint uuid
  Uuid deviceUuid    = node.getValueByPath<Uuid>("lib_device");
  Uuid footprintUuid = node.getValueByPath<Uuid>("lib_footprint");
  initDeviceAndPackageAndFootprint(deviceUuid, footprintUuid);

  // get position, rotation and mirrored
  mPosition   = Point(node.getChildByPath("position"));
  mRotation   = node.getValueByPath<Angle>("rotation");
  mIsMirrored = node.getValueByPath<bool>("mirror");

  // load attributes
  mAttributes.loadFromSExpression(node);  // can throw

  // load footprint
  mFootprint.reset(new BI_Footprint(*this, node));

  init();
}

BI_Device::BI_Device(Board& board, ComponentInstance& compInstance,
                     const Uuid& deviceUuid, const Uuid& footprintUuid,
                     const Point& position, const Angle& rotation, bool mirror)
  : BI_Base(board),
    mCompInstance(&compInstance),
    mLibDevice(nullptr),
    mLibPackage(nullptr),
    mLibFootprint(nullptr),
    mPosition(position),
    mRotation(rotation),
    mIsMirrored(mirror) {
  initDeviceAndPackageAndFootprint(deviceUuid, footprintUuid);

  // add attributes
  mAttributes = mLibDevice->getAttributes();

  // create footprint
  mFootprint.reset(new BI_Footprint(*this));

  init();
}

void BI_Device::initDeviceAndPackageAndFootprint(const Uuid& deviceUuid,
                                                 const Uuid& footprintUuid) {
  // get device from library
  mLibDevice = mBoard.getProject().getLibrary().getDevice(deviceUuid);
  if (!mLibDevice) {
    qDebug() << mCompInstance->getUuid();
    throw RuntimeError(__FILE__, __LINE__,
                       tr("No device with the UUID \"%1\" found in the "
                          "project's library.")
                           .arg(deviceUuid.toStr()));
  }
  // check if the device matches with the component
  if (mLibDevice->getComponentUuid() !=
      mCompInstance->getLibComponent().getUuid()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The device \"%1\" does not match with the component"
                "instance \"%2\".")
            .arg(mLibDevice->getUuid().toStr(),
                 mCompInstance->getUuid().toStr()));
  }
  // get package from library
  Uuid packageUuid = mLibDevice->getPackageUuid();
  mLibPackage      = mBoard.getProject().getLibrary().getPackage(packageUuid);
  if (!mLibPackage) {
    qDebug() << mCompInstance->getUuid();
    throw RuntimeError(__FILE__, __LINE__,
                       tr("No package with the UUID \"%1\" found in "
                          "the project's library.")
                           .arg(packageUuid.toStr()));
  }
  // get footprint from package
  mLibFootprint =
      mLibPackage->getFootprints().get(footprintUuid).get();  // can throw
}

void BI_Device::init() {
  // check pad-signal-map
  for (const library::DevicePadSignalMapItem& item :
       mLibDevice->getPadSignalMap()) {
    tl::optional<Uuid> signalUuid = item.getSignalUuid();
    if ((signalUuid) && (!mCompInstance->getSignalInstance(*signalUuid))) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Unknown signal \"%1\" found in device \"%2\"")
              .arg(signalUuid->toStr(), mLibDevice->getUuid().toStr()));
    }
  }

  // emit the "attributesChanged" signal when the board has emited it
  connect(&mBoard, &Board::attributesChanged, this,
          &BI_Device::attributesChanged);

  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_Device::~BI_Device() noexcept {
  mFootprint.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& BI_Device::getComponentInstanceUuid() const noexcept {
  return mCompInstance->getUuid();
}

bool BI_Device::isUsed() const noexcept {
  return mFootprint->isUsed();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Device::setPosition(const Point& pos) noexcept {
  if (pos != mPosition) {
    mPosition = pos;
    emit moved(mPosition);
  }
}

void BI_Device::setRotation(const Angle& rot) noexcept {
  if (rot != mRotation) {
    mRotation = rot;
    emit rotated(mRotation);
  }
}

void BI_Device::setIsMirrored(bool mirror) {
  if (mirror != mIsMirrored) {
    if (isUsed()) {
      throw LogicError(__FILE__, __LINE__);
    }
    mIsMirrored = mirror;
    emit mirrored(mIsMirrored);
  }
}

void BI_Device::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mCompInstance->registerDevice(*this);  // can throw
  auto sg = scopeGuard([&]() { mCompInstance->unregisterDevice(*this); });
  mFootprint->addToBoard();  // can throw
  sg.dismiss();
  BI_Base::addToBoard(nullptr);
  updateErcMessages();
}

void BI_Device::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mFootprint->removeFromBoard();  // can throw
  auto sg = scopeGuard([&]() { mFootprint->addToBoard(); });
  mCompInstance->unregisterDevice(*this);  // can throw
  sg.dismiss();
  BI_Base::removeFromBoard(nullptr);
  updateErcMessages();
}

void BI_Device::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

  root.appendChild(mCompInstance->getUuid());
  root.appendChild("lib_device", mLibDevice->getUuid(), true);
  root.appendChild("lib_footprint", mLibFootprint->getUuid(), true);
  root.appendChild(mPosition.serializeToDomElement("position"), true);
  root.appendChild("rotation", mRotation, false);
  root.appendChild("mirror", mIsMirrored, false);
  mAttributes.serialize(root);
  mFootprint->serialize(root);
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString BI_Device::getUserDefinedAttributeValue(const QString& key) const
    noexcept {
  if (std::shared_ptr<const Attribute> attr = mAttributes.find(key)) {
    return attr->getValueTr(true);
  } else {
    return QString();
  }
}

QString BI_Device::getBuiltInAttributeValue(const QString& key) const noexcept {
  if (key == QLatin1String("DEVICE")) {
    return *mLibDevice->getNames().value(getLocaleOrder());
  } else if (key == QLatin1String("PACKAGE")) {
    return *mLibPackage->getNames().value(getLocaleOrder());
  } else if (key == QLatin1String("FOOTPRINT")) {
    return *mLibFootprint->getNames().value(getLocaleOrder());
  } else {
    return QString();
  }
}

QVector<const AttributeProvider*> BI_Device::getAttributeProviderParents() const
    noexcept {
  return QVector<const AttributeProvider*>{&mBoard, mCompInstance};
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_Device::getGrabAreaScenePx() const noexcept {
  return mFootprint->getGrabAreaScenePx();
}

bool BI_Device::isSelectable() const noexcept {
  return mFootprint->isSelectable();
}

void BI_Device::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mFootprint->setSelected(selected);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BI_Device::checkAttributesValidity() const noexcept {
  if (mCompInstance == nullptr) return false;
  if (mLibDevice == nullptr) return false;
  if (mLibPackage == nullptr) return false;
  return true;
}

void BI_Device::updateErcMessages() noexcept {
}

const QStringList& BI_Device::getLocaleOrder() const noexcept {
  return getProject().getSettings().getLocaleOrder();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
