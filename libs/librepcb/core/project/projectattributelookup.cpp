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
#include "projectattributelookup.h"

#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/pkg/package.h"
#include "board/board.h"
#include "board/items/bi_device.h"
#include "circuit/circuit.h"
#include "circuit/componentinstance.h"
#include "project.h"
#include "schematic/items/si_symbol.h"
#include "schematic/schematic.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectAttributeLookup::ProjectAttributeLookup(
    const ProjectAttributeLookup& other) noexcept
  : mFunction(other.mFunction) {
}

ProjectAttributeLookup::ProjectAttributeLookup(const Project& obj) noexcept {
  QPointer<const Project> ptr(&obj);
  mFunction = [ptr](const QString& key) {
    QString value;
    if (ptr) {
      query(*ptr, key, value);  // Project
    }
    return value;
  };
}

ProjectAttributeLookup::ProjectAttributeLookup(
    const ComponentInstance& obj, QPointer<const BI_Device> device) noexcept {
  QPointer<const ComponentInstance> ptr(&obj);
  mFunction = [ptr, device](const QString& key) {
    QString value;
    if (ptr) {
      query(*ptr, key, value)  // Component
          || (device && query(*device, key, value))  // Device
          || query(ptr->getCircuit().getProject(), key, value);  // Project
    }
    return value;
  };
}

ProjectAttributeLookup::ProjectAttributeLookup(const Schematic& obj) noexcept {
  QPointer<const Schematic> ptr(&obj);
  mFunction = [ptr](const QString& key) {
    QString value;
    if (ptr) {
      query(*ptr, key, value)  // Schematic
          || query(ptr->getProject(), key, value);  // Project
    }
    return value;
  };
}

ProjectAttributeLookup::ProjectAttributeLookup(const Board& obj) noexcept {
  QPointer<const Board> ptr(&obj);
  mFunction = [ptr](const QString& key) {
    QString value;
    if (ptr) {
      query(*ptr, key, value)  // Board
          || query(ptr->getProject(), key, value);  // Project
    }
    return value;
  };
}

ProjectAttributeLookup::ProjectAttributeLookup(
    const SI_Symbol& obj, QPointer<const BI_Device> device) noexcept {
  QPointer<const SI_Symbol> ptr(&obj);
  mFunction = [ptr, device](const QString& key) {
    QString value;
    if (ptr) {
      query(*ptr, key, value)  // Symbol
          || query(ptr->getComponentInstance(), key, value)  // Component
          || (device && query(*device, key, value))  // Device
          || query(ptr->getSchematic(), key, value)  // Schematic
          || query(ptr->getProject(), key, value);  // Project
    }
    return value;
  };
}

ProjectAttributeLookup::ProjectAttributeLookup(const BI_Device& obj) noexcept {
  QPointer<const BI_Device> ptr(&obj);
  mFunction = [ptr](const QString& key) {
    QString value;
    if (ptr) {
      query(*ptr, key, value)  // Device
          || query(ptr->getComponentInstance(), key, value)  // Component
          || query(ptr->getBoard(), key, value)  // Board
          || query(ptr->getProject(), key, value);  // Project
    }
    return value;
  };
}

ProjectAttributeLookup::~ProjectAttributeLookup() noexcept {
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

QString ProjectAttributeLookup::operator()(const QString& key) const noexcept {
  Q_ASSERT(mFunction);
  return mFunction(key);
}

ProjectAttributeLookup& ProjectAttributeLookup::operator=(
    const ProjectAttributeLookup& rhs) noexcept {
  mFunction = rhs.mFunction;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool ProjectAttributeLookup::query(const Project& project, const QString& key,
                                   QString& value) noexcept {
  if (const auto& attr = project.getAttributes().find(key)) {
    value = attr->getValueTr(true);
    return true;
  } else if (key == QLatin1String("PROJECT")) {
    value = *project.getName();
    return true;
  } else if (key == QLatin1String("PROJECT_DIRPATH")) {
    value = project.getPath().toNative();
    return true;
  } else if (key == QLatin1String("PROJECT_BASENAME")) {
    value = project.getFilepath().getBasename();
    return true;
  } else if (key == QLatin1String("PROJECT_FILENAME")) {
    value = project.getFilepath().getFilename();
    return true;
  } else if (key == QLatin1String("PROJECT_FILEPATH")) {
    value = project.getFilepath().toNative();
    return true;
  } else if (key == QLatin1String("CREATED_DATE")) {
    value = project.getCreated().date().toString(Qt::ISODate);
    return true;
  } else if (key == QLatin1String("CREATED_TIME")) {
    value = project.getCreated().time().toString(Qt::ISODate);
    return true;
  } else if (key == QLatin1String("DATE")) {
    value = project.getDateTime().date().toString(Qt::ISODate);
    return true;
  } else if (key == QLatin1String("TIME")) {
    value = project.getDateTime().time().toString(Qt::ISODate);
    return true;
  } else if (key == QLatin1String("AUTHOR")) {
    value = project.getAuthor();
    return true;
  } else if (key == QLatin1String("VERSION")) {
    value = project.getVersion();
    return true;
  } else if (key == QLatin1String("PAGES")) {
    value = QString::number(project.getSchematics().count());
    return true;
  } else if (key == QLatin1String("PAGE_X_OF_Y")) {
    // Do not translate this, must be the same for every user!
    value = "Page {{PAGE}} of {{PAGES}}";
    return true;
  }
  return false;
}

bool ProjectAttributeLookup::query(const ComponentInstance& cmp,
                                   const QString& key,
                                   QString& value) noexcept {
  if (const auto& attr = cmp.getAttributes().find(key)) {
    value = attr->getValueTr(true);
    return true;
  } else if (key == QLatin1String("NAME")) {
    value = *cmp.getName();
    return true;
  } else if (key == QLatin1String("VALUE")) {
    value = cmp.getValue();
    return true;
  } else if (key == QLatin1String("COMPONENT")) {
    value = *cmp.getLibComponent().getNames().value(
        cmp.getCircuit().getProject().getLocaleOrder());
    return true;
  }
  return false;
}

bool ProjectAttributeLookup::query(const Schematic& schematic,
                                   const QString& key,
                                   QString& value) noexcept {
  if (key == QLatin1String("SHEET")) {
    value = *schematic.getName();
    return true;
  } else if (key == QLatin1String("PAGE")) {
    value = QString::number(
        schematic.getProject().getSchematicIndex(schematic) + 1);
    return true;
  }
  return false;
}

bool ProjectAttributeLookup::query(const Board& board, const QString& key,
                                   QString& value) noexcept {
  if (key == QLatin1String("BOARD")) {
    value = *board.getName();
    return true;
  } else if (key == QLatin1String("BOARD_DIRNAME")) {
    value = board.getDirectoryName();
    return true;
  } else if (key == QLatin1String("BOARD_INDEX")) {
    value = QString::number(board.getProject().getBoardIndex(board));
    return true;
  }
  return false;
}

bool ProjectAttributeLookup::query(const SI_Symbol& symbol, const QString& key,
                                   QString& value) noexcept {
  if (key == QLatin1String("NAME")) {
    value = symbol.getName();
    return true;
  }
  return false;
}

bool ProjectAttributeLookup::query(const BI_Device& device, const QString& key,
                                   QString& value) noexcept {
  if (const auto& attr = device.getAttributes().find(key)) {
    value = attr->getValueTr(true);
    return true;
  } else if (key == QLatin1String("DEVICE")) {
    value = *device.getLibDevice().getNames().value(
        device.getProject().getLocaleOrder());
    return true;
  } else if (key == QLatin1String("PACKAGE")) {
    value = *device.getLibPackage().getNames().value(
        device.getProject().getLocaleOrder());
    return true;
  } else if (key == QLatin1String("FOOTPRINT")) {
    value = *device.getLibFootprint().getNames().value(
        device.getProject().getLocaleOrder());
    return true;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
