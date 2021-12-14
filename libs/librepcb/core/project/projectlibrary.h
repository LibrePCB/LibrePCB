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

#ifndef LIBREPCB_PROJECT_PROJECTLIBRARY_H
#define LIBREPCB_PROJECT_PROJECTLIBRARY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/transactionaldirectory.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace library {
class Component;
class Device;
class LibraryBaseElement;
class Package;
class Symbol;
}  // namespace library

namespace project {

class Project;

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
  const QHash<Uuid, library::Symbol*>& getSymbols() const noexcept {
    return mSymbols;
  }
  const QHash<Uuid, library::Package*>& getPackages() const noexcept {
    return mPackages;
  }
  const QHash<Uuid, library::Component*>& getComponents() const noexcept {
    return mComponents;
  }
  const QHash<Uuid, library::Device*>& getDevices() const noexcept {
    return mDevices;
  }
  library::Symbol* getSymbol(const Uuid& uuid) const noexcept {
    return mSymbols.value(uuid);
  }
  library::Package* getPackage(const Uuid& uuid) const noexcept {
    return mPackages.value(uuid);
  }
  library::Component* getComponent(const Uuid& uuid) const noexcept {
    return mComponents.value(uuid);
  }
  library::Device* getDevice(const Uuid& uuid) const noexcept {
    return mDevices.value(uuid);
  }

  // Getters: Special Queries
  QHash<Uuid, library::Device*> getDevicesOfComponent(
      const Uuid& compUuid) const noexcept;

  // Add/Remove Methods
  void addSymbol(library::Symbol& s);
  void addPackage(library::Package& p);
  void addComponent(library::Component& c);
  void addDevice(library::Device& d);
  void removeSymbol(library::Symbol& s);
  void removePackage(library::Package& p);
  void removeComponent(library::Component& c);
  void removeDevice(library::Device& d);

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
  QHash<Uuid, library::Symbol*> mSymbols;
  QHash<Uuid, library::Package*> mPackages;
  QHash<Uuid, library::Component*> mComponents;
  QHash<Uuid, library::Device*> mDevices;

  QSet<library::LibraryBaseElement*> mAllElements;
  QSet<library::LibraryBaseElement*> mElementsToUpgrade;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
