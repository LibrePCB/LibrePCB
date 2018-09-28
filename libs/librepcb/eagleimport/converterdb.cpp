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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "converterdb.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ConverterDb::ConverterDb(const FilePath& ini) noexcept
  : mIniFile(ini.toStr(), QSettings::IniFormat) {
}

ConverterDb::~ConverterDb() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

Uuid ConverterDb::getSymbolUuid(const QString& symbolName) {
  return getOrCreateUuid("symbols", symbolName);
}

Uuid ConverterDb::getSymbolPinUuid(const Uuid&    symbolUuid,
                                   const QString& pinName) {
  return getOrCreateUuid("symbol_pins", symbolUuid.toStr(), pinName);
}

Uuid ConverterDb::getFootprintUuid(const QString& packageName) {
  return getOrCreateUuid("packages_to_footprints", packageName);
}

Uuid ConverterDb::getPackageUuid(const QString& packageName) {
  return getOrCreateUuid("packages_to_packages", packageName);
}

Uuid ConverterDb::getPackagePadUuid(const Uuid&    footprintUuid,
                                    const QString& padName) {
  return getOrCreateUuid("package_pads", footprintUuid.toStr(), padName);
}

Uuid ConverterDb::getComponentUuid(const QString& deviceSetName) {
  return getOrCreateUuid("devices_to_components", deviceSetName);
}

Uuid ConverterDb::getComponentSignalUuid(const Uuid&    componentUuid,
                                         const QString& gateName,
                                         const QString& pinName) {
  return getOrCreateUuid("gatepins_to_componentsignals", componentUuid.toStr(),
                         gateName % pinName);
}

Uuid ConverterDb::getSymbolVariantUuid(const Uuid& componentUuid) {
  return getOrCreateUuid("component_symbolvariants", componentUuid.toStr());
}

Uuid ConverterDb::getSymbolVariantItemUuid(const Uuid&    componentUuid,
                                           const QString& gateName) {
  return getOrCreateUuid("symbolgates_to_symbvaritems", componentUuid.toStr(),
                         gateName);
}

Uuid ConverterDb::getDeviceUuid(const QString& deviceSetName,
                                const QString& deviceName) {
  return getOrCreateUuid("devices_to_devices", deviceSetName, deviceName);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

Uuid ConverterDb::getOrCreateUuid(const QString& cat, const QString& key1,
                                  const QString& key2) {
  QString allowedChars(
      "_-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

  QString settingsKey = mLibFilePath.getFilename() % '_' % key1 % '_' % key2;
  settingsKey.replace("{", "");
  settingsKey.replace("}", "");
  settingsKey.replace(" ", "_");
  for (int i = 0; i < settingsKey.length(); i++) {
    if (!allowedChars.contains(settingsKey[i]))
      settingsKey.replace(
          i, 1,
          QString("__U%1__").arg(
              QString::number(settingsKey[i].unicode(), 16).toUpper()));
  }
  settingsKey.prepend(cat % '/');

  Uuid    uuid  = Uuid::createRandom();
  QString value = mIniFile.value(settingsKey).toString();
  if (!value.isEmpty()) uuid = Uuid::fromString(value);  // can throw
  mIniFile.setValue(settingsKey, uuid.toStr());
  return uuid;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb
