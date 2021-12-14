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

#include "../export/bom.h"
#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/pkg/package.h"
#include "board/board.h"
#include "board/items/bi_device.h"
#include "circuit/circuit.h"
#include "circuit/componentinstance.h"
#include "project.h"

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
  std::shared_ptr<Bom> bom = std::make_shared<Bom>(
      QStringList{"Value", "Device", "Package"} + mAdditionalAttributes);

  foreach (const ComponentInstance* cmpInst,
           mProject.getCircuit().getComponentInstances()) {
    if (cmpInst->getLibComponent().isSchematicOnly()) {
      continue;  // Don't export schematic-only components (e.g. sheet frames)
    }
    QString designator = *cmpInst->getName();
    QStringList attributes;
    QString devName;
    QString pkgName;
    if (board) {
      const BI_Device* device =
          board->getDeviceInstanceByComponentUuid(cmpInst->getUuid());
      if (device) {
        devName = *device->getLibDevice().getNames().getDefaultValue();
        pkgName = *device->getLibPackage().getNames().getDefaultValue();
      }
    }
    attributes.append(cmpInst->getValue(true));
    attributes.append(devName);
    attributes.append(pkgName);
    foreach (const QString& attribute, mAdditionalAttributes) {
      attributes.append(cmpInst->getAttributeValue(attribute));
    }
    bom->addItem(designator, attributes);
  }

  return bom;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
