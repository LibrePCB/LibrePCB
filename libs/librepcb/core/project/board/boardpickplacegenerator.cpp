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
#include "boardpickplacegenerator.h"

#include "../../export/pickplacedata.h"
#include "../../library/dev/device.h"
#include "../../library/pkg/package.h"
#include "../circuit/componentinstance.h"
#include "../project.h"
#include "../projectsettings.h"
#include "board.h"
#include "items/bi_device.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPickPlaceGenerator::BoardPickPlaceGenerator(const Board& board) noexcept
  : mBoard(board) {
}

BoardPickPlaceGenerator::~BoardPickPlaceGenerator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<PickPlaceData> BoardPickPlaceGenerator::generate() noexcept {
  // Assembly type map.
  static QMap<Package::AssemblyType, PickPlaceDataItem::Type> types = {
      {Package::AssemblyType::Tht, PickPlaceDataItem::Type::Tht},
      {Package::AssemblyType::Smt, PickPlaceDataItem::Type::Smt},
      {Package::AssemblyType::Mixed, PickPlaceDataItem::Type::Mixed},
      {Package::AssemblyType::Other, PickPlaceDataItem::Type::Other},
  };

  std::shared_ptr<PickPlaceData> data = std::make_shared<PickPlaceData>(
      *mBoard.getProject().getName(), mBoard.getProject().getVersion(),
      *mBoard.getName());
  const QStringList& locale =
      mBoard.getProject().getSettings().getLocaleOrder();

  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    // Skip devices which are considered as no device to be mounted.
    auto typeIt = types.find(device->getLibPackage().getAssemblyType(true));
    if (typeIt == types.end()) {
      continue;
    }

    QString designator = *device->getComponentInstance().getName();
    QString value = device->getComponentInstance().getValue(true).trimmed();
    QString deviceName = *device->getLibDevice().getNames().value(locale);
    QString packageName = *device->getLibPackage().getNames().value(locale);
    Point position = device->getPosition();
    Angle rotation = device->getRotation();
    PickPlaceDataItem::BoardSide boardSide = device->getMirrored()
        ? PickPlaceDataItem::BoardSide::Bottom
        : PickPlaceDataItem::BoardSide::Top;
    data->addItem(PickPlaceDataItem(designator, value, deviceName, packageName,
                                    position, rotation, boardSide, *typeIt));
  }

  return data;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
