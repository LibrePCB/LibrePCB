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

#ifndef LIBREPCB_CORE_PROJECTLIBRARY_H
#define LIBREPCB_CORE_PROJECTLIBRARY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/transactionaldirectory.h"
#include "../types/uuid.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class Device;
class LibraryBaseElement;
class Package;
class Project;
class Symbol;

/*******************************************************************************
 *  Class ProjectLibrary
 ******************************************************************************/

/**
 * @brief The ProjectLibrary class
 */
class ProjectLibrary final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectLibrary() = delete;
  ProjectLibrary(const ProjectLibrary& other) = delete;
  ProjectLibrary(std::unique_ptr<TransactionalDirectory> directory);
  ~ProjectLibrary() noexcept;

  // Getters: Library Elements
  const QHash<Uuid, Symbol*>& getSymbols() const noexcept { return mSymbols; }
  const QHash<Uuid, Package*>& getPackages() const noexcept {
    return mPackages;
  }
  const QHash<Uuid, Component*>& getComponents() const noexcept {
    return mComponents;
  }
  const QHash<Uuid, Device*>& getDevices() const noexcept { return mDevices; }
  Symbol* getSymbol(const Uuid& uuid) const noexcept {
    return mSymbols.value(uuid);
  }
  Package* getPackage(const Uuid& uuid) const noexcept {
    return mPackages.value(uuid);
  }
  Component* getComponent(const Uuid& uuid) const noexcept {
    return mComponents.value(uuid);
  }
  Device* getDevice(const Uuid& uuid) const noexcept {
    return mDevices.value(uuid);
  }

  // Getters: Special Queries
  QHash<Uuid, Device*> getDevicesOfComponent(const Uuid& compUuid) const
      noexcept;

  // Add/Remove Methods
  void addSymbol(Symbol& s);
  void addPackage(Package& p);
  void addComponent(Component& c);
  void addDevice(Device& d);
  void removeSymbol(Symbol& s);
  void removePackage(Package& p);
  void removeComponent(Component& c);
  void removeDevice(Device& d);

  // General Methods
  void save();

  // Operator Overloadings
  ProjectLibrary& operator=(const ProjectLibrary& rhs) = delete;

private:
  // Private Methods
  template <typename ElementType>
  void loadElements(const QString& dirname, const QString& type,
                    QHash<Uuid, ElementType*>& elementList);
  template <typename ElementType>
  void addElement(ElementType& element, QHash<Uuid, ElementType*>& elementList);
  template <typename ElementType>
  void removeElement(ElementType& element,
                     QHash<Uuid, ElementType*>& elementList);

  // General
  std::unique_ptr<TransactionalDirectory> mDirectory;

  // The currently added library elements
  QHash<Uuid, Symbol*> mSymbols;
  QHash<Uuid, Package*> mPackages;
  QHash<Uuid, Component*> mComponents;
  QHash<Uuid, Device*> mDevices;

  QSet<LibraryBaseElement*> mAllElements;
  QSet<LibraryBaseElement*> mElementsToUpgrade;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
