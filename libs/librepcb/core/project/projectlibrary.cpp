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
#include "projectlibrary.h"

#include "../fileio/transactionalfilesystem.h"
#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/pkg/package.h"
#include "../library/sym/symbol.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectLibrary::ProjectLibrary(
    std::unique_ptr<TransactionalDirectory> directory)
  : mDirectory(std::move(directory)) {
}

ProjectLibrary::~ProjectLibrary() noexcept {
  qDeleteAll(mAllElements);
  mAllElements.clear();
}

/*******************************************************************************
 *  Getters: Special Queries
 ******************************************************************************/

QHash<Uuid, Device*> ProjectLibrary::getDevicesOfComponent(
    const Uuid& compUuid) const noexcept {
  QHash<Uuid, Device*> list;
  foreach (Device* device, mDevices) {
    if (device->getComponentUuid() == compUuid) {
      list.insert(device->getUuid(), device);
    }
  }
  return list;
}

/*******************************************************************************
 *  Add/Remove Methods
 ******************************************************************************/

void ProjectLibrary::addSymbol(Symbol& s) {
  addElement<Symbol>(s, mSymbols);
}

void ProjectLibrary::addPackage(Package& p) {
  addElement<Package>(p, mPackages);
}

void ProjectLibrary::addComponent(Component& c) {
  addElement<Component>(c, mComponents);
}

void ProjectLibrary::addDevice(Device& d) {
  addElement<Device>(d, mDevices);
}

void ProjectLibrary::removeSymbol(Symbol& s) {
  removeElement<Symbol>(s, mSymbols);
}

void ProjectLibrary::removePackage(Package& p) {
  removeElement<Package>(p, mPackages);
}

void ProjectLibrary::removeComponent(Component& c) {
  removeElement<Component>(c, mComponents);
}

void ProjectLibrary::removeDevice(Device& d) {
  removeElement<Device>(d, mDevices);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename ElementType>
void ProjectLibrary::addElement(ElementType& element,
                                QHash<Uuid, ElementType*>& elementList) {
  if (elementList.contains(element.getUuid())) {
    throw LogicError(__FILE__, __LINE__,
                     QString("There is already an element with the same "
                             "UUID in the project's library: %1")
                         .arg(element.getUuid().toStr()));
  }

  // Copy files, if necessary. In any case, the file format will be upgraded
  // as well.
  if (element.getDirectory().getFileSystem() != mDirectory->getFileSystem()) {
    TransactionalDirectory dir(*mDirectory, element.getShortElementName());
    element.saveIntoParentDirectory(dir);  // can throw
  } else {
    element.save();  // can throw
  }

  elementList.insert(element.getUuid(), &element);
  mAllElements.insert(&element);
}

template <typename ElementType>
void ProjectLibrary::removeElement(ElementType& element,
                                   QHash<Uuid, ElementType*>& elementList) {
  Q_ASSERT(elementList.value(element.getUuid()) == &element);
  Q_ASSERT(mAllElements.contains(&element));
  TransactionalDirectory tmpDir;
  element.moveIntoParentDirectory(tmpDir);  // can throw
  elementList.remove(element.getUuid());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
