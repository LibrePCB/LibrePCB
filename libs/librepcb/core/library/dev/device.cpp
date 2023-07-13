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

#include "../../serialization/fileformatmigration.h"
#include "devicecheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

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
    mPadSignalMap(),
    mAttributes(),
    mParts() {
}

Device::Device(std::unique_ptr<TransactionalDirectory> directory,
               const SExpression& root)
  : LibraryElement(getShortElementName(), getLongElementName(), true,
                   std::move(directory), root),
    mComponentUuid(deserialize<Uuid>(root.getChild("component/@0"))),
    mPackageUuid(deserialize<Uuid>(root.getChild("package/@0"))),
    mPadSignalMap(root),
    mAttributes(root),
    mParts(root) {
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

RuleCheckMessageList Device::runChecks() const {
  DeviceCheck check(*this);
  return check.runChecks();  // can throw
}

std::unique_ptr<Device> Device::open(
    std::unique_ptr<TransactionalDirectory> directory) {
  Q_ASSERT(directory);

  // Upgrade file format, if needed.
  const Version fileFormat =
      readFileFormat(*directory, ".librepcb-" % getShortElementName());
  const auto migrations = FileFormatMigration::getMigrations(fileFormat);
  for (auto migration : migrations) {
    migration->upgradeDevice(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const SExpression root = SExpression::parse(directory->read(fileName),
                                              directory->getAbsPath(fileName));
  std::unique_ptr<Device> obj(new Device(std::move(directory), root));
  if (!migrations.isEmpty()) {
    obj->removeObsoleteMessageApprovals();
  }
  return obj;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Device::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  root.ensureLineBreak();
  root.appendChild("component", mComponentUuid);
  root.ensureLineBreak();
  root.appendChild("package", mPackageUuid);
  root.ensureLineBreak();
  mPadSignalMap.sortedByUuid().serialize(root);
  root.ensureLineBreak();
  mAttributes.serialize(root);
  root.ensureLineBreak();
  mParts.serialize(root);
  root.ensureLineBreak();
  serializeMessageApprovals(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
