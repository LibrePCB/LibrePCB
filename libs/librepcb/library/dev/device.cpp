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
#include "device.h"

#include "devicecheck.h"

#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Device::Device(const Uuid& uuid, const Version& version, const QString& author,
               const ElementName& name_en_US, const QString& description_en_US,
               const QString& keywords_en_US, const Uuid& component,
               const Uuid& package)
  : LibraryElement(getShortElementName(), getLongElementName(), uuid, version,
                   author, name_en_US, description_en_US, keywords_en_US),
    mComponentUuid(component),
    mPackageUuid(package),
    mAttributes(),
    mPadSignalMap() {
}

Device::Device(std::unique_ptr<TransactionalDirectory> directory)
  : LibraryElement(std::move(directory), getShortElementName(),
                   getLongElementName()),
    mComponentUuid(
        deserialize<Uuid>(mLoadingFileDocument.getChild("component/@0"))),
    mPackageUuid(
        deserialize<Uuid>(mLoadingFileDocument.getChild("package/@0"))),
    mAttributes(mLoadingFileDocument),
    mPadSignalMap(mLoadingFileDocument) {
  cleanupAfterLoadingElementFromFile();
}

Device::~Device() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Device::setComponentUuid(const Uuid& uuid) noexcept {
  if (uuid == mComponentUuid) return;
  mComponentUuid = uuid;
  emit componentUuidChanged(mComponentUuid);
}

void Device::setPackageUuid(const Uuid& uuid) noexcept {
  if (uuid == mPackageUuid) return;
  mPackageUuid = uuid;
  emit packageUuidChanged(mPackageUuid);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList Device::runChecks() const {
  DeviceCheck check(*this);
  return check.runChecks();  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Device::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  root.appendChild("component", mComponentUuid, true);
  root.appendChild("package", mPackageUuid, true);
  mAttributes.serialize(root);
  mPadSignalMap.sortedByUuid().serialize(root);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
