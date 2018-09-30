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
#include "projectlibrary.h"

#include "../project.h"

#include <librepcb/common/application.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/sym/symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

using namespace library;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectLibrary::ProjectLibrary(const FilePath& libDir, bool restore,
                               bool readOnly)
  : mLibraryPath(libDir),
    mBackupPath(libDir.toStr() % '~'),
    mTmpDir(FilePath::getRandomTempPath()) {
  qDebug() << "load project library...";

  if ((!mLibraryPath.isExistingDir()) && (!readOnly)) {
    FileUtils::makePath(mLibraryPath);  // can throw
  }

  try {
    // Load all library elements
    const FilePath& dirToLoad =
        restore && mBackupPath.isExistingDir() ? mBackupPath : mLibraryPath;
    loadElements<Symbol>(dirToLoad.getPathTo("sym"), "symbols", mSymbols);
    loadElements<Package>(dirToLoad.getPathTo("pkg"), "packages", mPackages);
    loadElements<Component>(dirToLoad.getPathTo("cmp"), "components",
                            mComponents);
    loadElements<Device>(dirToLoad.getPathTo("dev"), "devices", mDevices);
  } catch (const Exception&) {
    qDeleteAll(mAllElements);
    mAllElements.clear();
    throw;
  }

  qDebug() << "project library successfully loaded!";
}

ProjectLibrary::~ProjectLibrary() noexcept {
  // Delete all library elements.
  qDeleteAll(mAllElements);
  mAllElements.clear();

  // Remove backup directory.
  if (!QDir(mBackupPath.toStr()).removeRecursively()) {
    qWarning() << "Could not remove backup directory" << mBackupPath.toStr();
  }

  // Remove temporary directory.
  if (!QDir(mTmpDir.toStr()).removeRecursively()) {
    qWarning() << "Could not remove temporary directory" << mTmpDir.toStr();
  }
}

/*******************************************************************************
 *  Getters: Special Queries
 ******************************************************************************/

QHash<Uuid, library::Device*> ProjectLibrary::getDevicesOfComponent(
    const Uuid& compUuid) const noexcept {
  QHash<Uuid, library::Device*> list;
  foreach (library::Device* device, mDevices) {
    if (device->getComponentUuid() == compUuid) {
      list.insert(device->getUuid(), device);
    }
  }
  return list;
}

/*******************************************************************************
 *  Add/Remove Methods
 ******************************************************************************/

void ProjectLibrary::addSymbol(library::Symbol& s) {
  addElement<Symbol>(s, mSymbols);
}

void ProjectLibrary::addPackage(library::Package& p) {
  addElement<Package>(p, mPackages);
}

void ProjectLibrary::addComponent(library::Component& c) {
  addElement<Component>(c, mComponents);
}

void ProjectLibrary::addDevice(library::Device& d) {
  addElement<Device>(d, mDevices);
}

void ProjectLibrary::removeSymbol(library::Symbol& s) {
  removeElement<Symbol>(s, mSymbols);
}

void ProjectLibrary::removePackage(library::Package& p) {
  removeElement<Package>(p, mPackages);
}

void ProjectLibrary::removeComponent(library::Component& c) {
  removeElement<Component>(c, mComponents);
}

void ProjectLibrary::removeDevice(library::Device& d) {
  removeElement<Device>(d, mDevices);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool ProjectLibrary::save(bool toOriginal, QStringList& errors) noexcept {
  bool success = true;

  QSet<LibraryBaseElement*>  currentElements = getCurrentElements();
  QSet<LibraryBaseElement*>& savedElements =
      toOriginal ? mSavedToOriginal : mSavedToTemporary;
  const FilePath& libDir = toOriginal ? mLibraryPath : mBackupPath;

  // Remove no longer needed elements.
  foreach (LibraryBaseElement* element,
           mLoadedElements + savedElements - currentElements) {
    FilePath dir = libDir.getPathTo(element->getShortElementName())
                       .getPathTo(element->getUuid().toStr());
    try {
      FileUtils::removeDirRecursively(dir);  // can throw
      savedElements.remove(element);
    } catch (const Exception& e) {
      success = false;
      errors.append(e.getMsg());
    }
  }

  // Add new elements and upgrade loaded elements.
  foreach (LibraryBaseElement* element, currentElements - savedElements) {
    FilePath dir = libDir.getPathTo(element->getShortElementName())
                       .getPathTo(element->getUuid().toStr());
    try {
      if (toOriginal && mLoadedElements.contains(element)) {
        // Upgrade library element to latest file format version.
        element->save();  // can throw
        mLoadedElements.remove(element);
      }
      if (dir.isExistingDir()) {
        // Avoid copy failure caused by already existing directory.
        FileUtils::removeDirRecursively(dir);
      }
      FileUtils::copyDirRecursively(element->getFilePath(), dir);  // can throw
      savedElements.insert(element);
    } catch (const Exception& e) {
      success = false;
      errors.append(e.getMsg());
    }
  }

  return success;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QSet<LibraryBaseElement*> ProjectLibrary::getCurrentElements() const noexcept {
  QSet<LibraryBaseElement*> currentElements;
  foreach (auto element, mSymbols) { currentElements.insert(element); }
  foreach (auto element, mPackages) { currentElements.insert(element); }
  foreach (auto element, mComponents) { currentElements.insert(element); }
  foreach (auto element, mDevices) { currentElements.insert(element); }
  return currentElements;
}

template <typename ElementType>
void ProjectLibrary::loadElements(const FilePath&            directory,
                                  const QString&             type,
                                  QHash<Uuid, ElementType*>& elementList) {
  QDir dir(directory.toStr());

  // search all subdirectories which have a valid UUID as directory name
  dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable);
  dir.setNameFilters(QStringList()
                     << QString("*.%1").arg(directory.getBasename()));
  foreach (const QString& dirname, dir.entryList()) {
    FilePath subdirPath(directory.getPathTo(dirname));

    // check if directory is a valid library element
    if (!LibraryBaseElement::isValidElementDirectory<ElementType>(subdirPath)) {
      if (subdirPath.isEmptyDir()) {
        qInfo() << "Empty library element directory will be removed:"
                << subdirPath.toNative();
        QDir(subdirPath.toStr()).removeRecursively();
      } else {
        qWarning() << "Found an invalid directory in the library:"
                   << subdirPath.toNative();
      }
      continue;
    }

    // Copy element to temporary directory to decouple it from the project
    // library.
    FilePath elementDir =
        mTmpDir.getPathTo(QString::number(qrand())).getPathTo(dirname);
    FileUtils::copyDirRecursively(subdirPath, elementDir);  // can throw

    // load the library element
    ElementType* element = new ElementType(elementDir, false);  // can throw
    if (elementList.contains(element->getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString(tr("There are multiple library elements with the same "
                     "UUID in the directory \"%1\""))
              .arg(subdirPath.toNative()));
    }

    // everything is ok -> update members
    elementList.insert(element->getUuid(), element);
    mAllElements.insert(element);
    mLoadedElements.insert(element);
  }

  qDebug() << "successfully loaded" << elementList.count() << qPrintable(type);
}

template <typename ElementType>
void ProjectLibrary::addElement(ElementType&               element,
                                QHash<Uuid, ElementType*>& elementList) {
  if (elementList.contains(element.getUuid())) {
    throw LogicError(__FILE__, __LINE__,
                     QString(tr("There is already an element with the same "
                                "UUID in the project's library: %1"))
                         .arg(element.getUuid().toStr()));
  }
  if (!mAllElements.contains(&element)) {
    // copy from workspace *immediately* to freeze/backup their state
    element.saveIntoParentDirectory(
        mTmpDir.getPathTo(QString::number(qrand())));  // can throw
    mAllElements.insert(&element);
  }
  elementList.insert(element.getUuid(), &element);
}

template <typename ElementType>
void ProjectLibrary::removeElement(ElementType&               element,
                                   QHash<Uuid, ElementType*>& elementList) {
  Q_ASSERT(elementList.value(element.getUuid()) == &element);
  Q_ASSERT(mAllElements.contains(&element));
  elementList.remove(element.getUuid());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
