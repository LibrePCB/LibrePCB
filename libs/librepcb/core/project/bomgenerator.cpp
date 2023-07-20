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
#include "bomgenerator.h"

#include "../attribute/attributesubstitutor.h"
#include "../export/bom.h"
#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/pkg/package.h"
#include "board/board.h"
#include "board/items/bi_device.h"
#include "circuit/circuit.h"
#include "circuit/componentinstance.h"
#include "project.h"
#include "projectattributelookup.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BomGenerator::BomGenerator(const Project& project) noexcept
  : mProject(project), mAdditionalAttributes() {
}

BomGenerator::~BomGenerator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<Bom> BomGenerator::generate(const Board* board) noexcept {
  // Parse custom attributes.
  QStringList customCommonAttributes;
  QStringList customPartAttributes;
  foreach (const QString& attribute, mAdditionalAttributes) {
    if (attribute.endsWith("[]")) {
      customPartAttributes.append(attribute.chopped(2));
    } else {
      customCommonAttributes.append(attribute);
    }
  }

  // Collect items.
  struct PartItem {
    QString mpn;
    QString manufacturer;
    QString value;
    QStringList attributes;  // Those from customPartAttributes.
  };
  struct ComponentItem {
    QString designator;
    QVector<PartItem> parts;
    QString pkgName;
    QStringList attributes;  // Those from customCommonAttributes.
  };
  QList<ComponentItem> items;
  int maxPartNumber = 1;
  foreach (const ComponentInstance* cmpInst,
           mProject.getCircuit().getComponentInstances()) {
    ProjectAttributeLookup lookup(*cmpInst, nullptr, nullptr);
    const BI_Device* device = nullptr;
    QVector<std::shared_ptr<const Part>> parts;
    if (board) {
      device = board->getDeviceInstanceByComponentUuid(cmpInst->getUuid());
      if (device) {
        lookup = ProjectAttributeLookup(*device, nullptr);
        parts = device->getParts();
      }
    } else {
      parts = cmpInst->getParts();
    }

    ComponentItem item;
    item.designator = *cmpInst->getName();
    item.pkgName = board ? lookup("PACKAGE") : "N/A";
    foreach (auto part, parts) {
      auto lookup = device ? ProjectAttributeLookup(*device, part)
                           : ProjectAttributeLookup(*cmpInst, nullptr, part);
      PartItem partItem;
      partItem.mpn = *part->getMpn();
      partItem.manufacturer = *part->getManufacturer();
      partItem.value = AttributeSubstitutor::substitute(lookup("VALUE"), lookup)
                           .simplified();
      // Remove redundant information from the value since it could
      // lead to confusion.
      if (!partItem.mpn.isEmpty()) {
        removeSubString(partItem.value, partItem.mpn);
        removeSubString(partItem.value, lookup("DEVICE"));
        removeSubString(partItem.value, lookup("COMPONENT"));
      }
      if (!partItem.manufacturer.isEmpty()) {
        removeSubString(partItem.value, partItem.manufacturer);
      }
      foreach (const QString& attribute, customPartAttributes) {
        partItem.attributes.append(
            AttributeSubstitutor::substitute(lookup(attribute), lookup));
      }
      item.parts.append(partItem);
    }
    foreach (const QString& attribute, customCommonAttributes) {
      item.attributes.append(
          AttributeSubstitutor::substitute(lookup(attribute), lookup));
    }
    maxPartNumber = std::max(maxPartNumber, item.parts.count());
    if (item.parts.isEmpty()) {
      continue;  // Skip components which have no assembly information (=DNM).
    }
    items.append(item);
  }

  // Build BOM header.
  QStringList columns{"Package"};
  columns += customCommonAttributes;
  for (int i = 0; i < maxPartNumber; ++i) {
    const QString suffix = (i > 0) ? QString("[%1]").arg(i + 1) : QString();
    columns.append("Value" % suffix);
    columns.append("MPN" % suffix);
    columns.append("Manufacturer" % suffix);
    foreach (const QString& attribute, customPartAttributes) {
      columns.append(attribute % suffix);
    }
  }

  // Generate BOM.
  std::shared_ptr<Bom> bom = std::make_shared<Bom>(columns);
  foreach (const ComponentItem& item, items) {
    QStringList attributes;
    attributes.append(item.pkgName);
    attributes += item.attributes;
    for (int i = 0; i < maxPartNumber; ++i) {
      const PartItem part = item.parts.value(i);
      attributes.append(part.value);
      attributes.append(part.mpn);
      attributes.append(part.manufacturer);
      attributes += part.attributes;
      for (int k = part.attributes.count(); k < customPartAttributes.count();
           ++k) {
        attributes.append(QString());
      }
    }
    bom->addItem(item.designator, attributes);
  }

  return bom;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BomGenerator::removeSubString(QString& str,
                                   const QString& substr) noexcept {
  str.replace(
      QRegularExpression(
          QString("(^|\\s)%1($|\\s)").arg(QRegularExpression::escape(substr))),
      " ");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
