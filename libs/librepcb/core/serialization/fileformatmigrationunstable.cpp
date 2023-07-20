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
#include "fileformatmigrationunstable.h"

#include "../application.h"
#include "../fileio/transactionaldirectory.h"
#include "../types/elementname.h"
#include "../types/simplestring.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileFormatMigrationUnstable::FileFormatMigrationUnstable(
    QObject* parent) noexcept
  : FileFormatMigrationV01(parent) {
  mFromVersion = mToVersion;  // Clearly distinguish from the base class.
}

FileFormatMigrationUnstable::~FileFormatMigrationUnstable() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FileFormatMigrationUnstable::upgradeComponentCategory(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradePackageCategory(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeSymbol(TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradePackage(TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeComponent(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeDevice(TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeLibrary(TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeWorkspaceData(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FileFormatMigrationUnstable::upgradeSettings(SExpression& root) {
  Q_UNUSED(root);
  root.appendChild("default_lock_component_assembly",
                   SExpression::createToken("false"));
}

void FileFormatMigrationUnstable::upgradeCircuit(SExpression& root,
                                                 ProjectContext& context) {
  Q_UNUSED(root);

  // Add assembly options & parts to components.
  foreach (SExpression* cmpNode, root.getChildren("component")) {
    if (!cmpNode->tryGetChild("lib_device")) {
      continue;  // already migrated
    }

    const Uuid cmpUuid = deserialize<Uuid>(cmpNode->getChild("@0"));
    const SExpression& libDevNode = cmpNode->getChild("lib_device");
    QSet<Uuid> libDeviceUuids = context.devicesUsedInBoards.value(cmpUuid);
    if (auto u = deserialize<tl::optional<Uuid>>(libDevNode.getChild("@0"))) {
      libDeviceUuids.insert(*u);
    }

    if (!libDeviceUuids.isEmpty()) {
      QString mpn, manufacturer;
      QVector<SExpression*> consumedAttributes;
      const QList<SExpression*> attributes = cmpNode->getChildren("attribute");
      for (int i = attributes.count() - 1; i >= 0; --i) {
        const QString key = attributes.at(i)->getChild("@0").getValue();
        if (key == "MPN") {
          mpn = *cleanSimpleString(
              attributes.at(i)->getChild("value/@0").getValue());
          consumedAttributes.append(attributes.at(i));
        } else if (key == "MANUFACTURER") {
          manufacturer = *cleanSimpleString(
              attributes.at(i)->getChild("value/@0").getValue());
          consumedAttributes.append(attributes.at(i));
        }
      }
      foreach (SExpression* attrNode, consumedAttributes) {
        cmpNode->removeChild(*attrNode);
      }

      foreach (const Uuid& devUuid, libDeviceUuids) {
        SExpression& devNode = cmpNode->appendList("device");
        devNode.appendChild(SExpression::createToken(devUuid.toStr()));
        if ((!mpn.isEmpty()) || (!manufacturer.isEmpty())) {
          SExpression& partNode = devNode.appendList("part");
          partNode.appendChild(mpn);
          partNode.appendChild("manufacturer", manufacturer);
        }
      }
      ++context.componentsWithAssemblyOptions;
    }

    cmpNode->removeChild(libDevNode);
    cmpNode->appendChild("lock_assembly", SExpression::createToken("false"));
  }
}

void FileFormatMigrationUnstable::upgradeErc(SExpression& root,
                                             ProjectContext& context) {
  Q_UNUSED(root);
  Q_UNUSED(context);
}

void FileFormatMigrationUnstable::upgradeSchematic(SExpression& root,
                                                   ProjectContext& context) {
  Q_UNUSED(root);
  Q_UNUSED(context);
}

void FileFormatMigrationUnstable::upgradeBoard(SExpression& root,
                                               ProjectContext& context) {
  Q_UNUSED(root);
  Q_UNUSED(context);
}

void FileFormatMigrationUnstable::upgradeBoardUserSettings(SExpression& root) {
  Q_UNUSED(root);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
