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

#include "../../attribute/attributesubstitutor.h"
#include "../../export/pickplacedata.h"
#include "../../library/dev/device.h"
#include "../../library/pkg/package.h"
#include "../circuit/componentinstance.h"
#include "../project.h"
#include "../projectattributelookup.h"
#include "board.h"
#include "items/bi_device.h"
#include "items/bi_pad.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPickPlaceGenerator::BoardPickPlaceGenerator(
    const Board& board, const Uuid& assemblyVariant) noexcept
  : mBoard(board), mAssemblyVariant(assemblyVariant) {
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
      *mBoard.getProject().getName(), *mBoard.getProject().getVersion(),
      *mBoard.getName());
  const QStringList& locale = mBoard.getProject().getLocaleOrder();

  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    auto part = device->getParts(mAssemblyVariant).value(0);
    ProjectAttributeLookup lookup(*device, part);
    QList<PickPlaceDataItem> items;
    const QString designator = *device->getComponentInstance().getName();
    const QString value =
        AttributeSubstitutor::substitute("{{MPN or VALUE or DEVICE}}", lookup)
            .simplified();
    const QString devName = *device->getLibDevice().getNames().value(locale);
    const QString pkgName = *device->getLibPackage().getNames().value(locale);

    // Determine fiducials to be exported.
    foreach (const BI_Pad* pad, device->getPads()) {
      if (pad->getLibPad().getFunctionIsFiducial()) {
        QVector<PickPlaceDataItem::BoardSide> sides;
        if (pad->isOnLayer(Layer::topCopper())) {
          sides.append(PickPlaceDataItem::BoardSide::Top);
        }
        if (pad->isOnLayer(Layer::botCopper())) {
          sides.append(PickPlaceDataItem::BoardSide::Bottom);
        }
        const Angle rotation =
            pad->getMirrored() ? -pad->getRotation() : pad->getRotation();
        foreach (const auto side, sides) {
          items.append(PickPlaceDataItem(
              designator, value, devName, pkgName, pad->getPosition(), rotation,
              side, PickPlaceDataItem::Type::Fiducial, true));
        }
      }
    }

    // Ensure unique designators for pad items if there are multiple.
    if (items.count() > 1) {
      for (int i = 0; i < items.count(); ++i) {
        items[i].setDesignator(items[i].getDesignator() % ":" %
                               QString::number(i + 1));
      }
    }

    // Export device only if it is contained in the assembly variant.
    const Point position = device->getPosition();
    const Angle rotation =
        device->getMirrored() ? -device->getRotation() : device->getRotation();
    const PickPlaceDataItem::BoardSide boardSide = device->getMirrored()
        ? PickPlaceDataItem::BoardSide::Bottom
        : PickPlaceDataItem::BoardSide::Top;
    const PickPlaceDataItem::Type assemblyType =
        types.value(device->getLibPackage().getAssemblyType(true),
                    PickPlaceDataItem::Type::Other);
    items.append(PickPlaceDataItem(designator, value, devName, pkgName,
                                   position, rotation, boardSide, assemblyType,
                                   part ? true : false));

    // Add all items.
    for (const PickPlaceDataItem& item : items) {
      data->addItem(item);
    }
  }

  return data;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
