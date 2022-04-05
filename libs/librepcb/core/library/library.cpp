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
#include "library.h"

#include "../fileio/transactionalfilesystem.h"
#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"
#include "cat/componentcategory.h"
#include "cat/packagecategory.h"
#include "cmp/component.h"
#include "dev/device.h"
#include "pkg/package.h"
#include "sym/symbol.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Library::Library(const Uuid& uuid, const Version& version,
                 const QString& author, const ElementName& name_en_US,
                 const QString& description_en_US,
                 const QString& keywords_en_US)
  : LibraryBaseElement(false, getShortElementName(), getLongElementName(), uuid,
                       version, author, name_en_US, description_en_US,
                       keywords_en_US) {
}

Library::Library(std::unique_ptr<TransactionalDirectory> directory)
  : LibraryBaseElement(std::move(directory), false, "lib", "library") {
  // check directory suffix
  if (mDirectory->getAbsPath().getSuffix() != "lplib") {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The library directory does not have the "
                          "suffix '.lplib':\n\n%1")
                           .arg(mDirectory->getAbsPath().toNative()));
  }

  // read properties
  // Note: Don't use SExpression::getValueByPath<QUrl>() because it would throw
  // an exception if the URL is empty, which is actually legal in this case.
  mUrl = QUrl(mLoadingFileDocument.getChild("url/@0").getValue(),
              QUrl::StrictMode);

  // read dependency UUIDs
  foreach (const SExpression& node,
           mLoadingFileDocument.getChildren("dependency")) {
    mDependencies.insert(
        deserialize<Uuid>(node.getChild("@0"), mLoadingFileFormat));
  }

  // load image if available
  if (mDirectory->fileExists("library.png")) {
    mIcon = mDirectory->read("library.png");  // can throw
  }

  cleanupAfterLoadingElementFromFile();
}

Library::~Library() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

template <typename ElementType>
QString Library::getElementsDirectoryName() const noexcept {
  return ElementType::getShortElementName();
}

// explicit template instantiations
template QString Library::getElementsDirectoryName<ComponentCategory>() const
    noexcept;
template QString Library::getElementsDirectoryName<PackageCategory>() const
    noexcept;
template QString Library::getElementsDirectoryName<Symbol>() const noexcept;
template QString Library::getElementsDirectoryName<Package>() const noexcept;
template QString Library::getElementsDirectoryName<Component>() const noexcept;
template QString Library::getElementsDirectoryName<Device>() const noexcept;

QPixmap Library::getIconAsPixmap() const noexcept {
  QPixmap p;
  p.loadFromData(mIcon, "png");
  return p;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Library::save() {
  LibraryBaseElement::save();  // can throw

  // Save icon.
  if (mIcon.isEmpty() && mDirectory->fileExists("library.png")) {
    mDirectory->removeFile("library.png");  // can throw
  } else if (!mIcon.isEmpty()) {
    mDirectory->write("library.png", mIcon);  // can throw
  }
}

void Library::moveTo(TransactionalDirectory& dest) {
  // check directory suffix
  if (dest.getAbsPath().getSuffix() != "lplib") {
    qDebug() << dest.getAbsPath().toNative();
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("A library directory name must have the suffix '.lplib'."));
  }

  // move the element
  LibraryBaseElement::moveTo(dest);
}

template <typename ElementType>
QStringList Library::searchForElements() const noexcept {
  QStringList list;
  QString subdir = getElementsDirectoryName<ElementType>();
  foreach (const QString& dirname, mDirectory->getDirs(subdir)) {
    QString dirPath = subdir % "/" % dirname;
    if (isValidElementDirectory<ElementType>(*mDirectory, dirPath)) {
      list.append(dirPath);
    } else {
      qWarning() << "Directory is not a valid library element:"
                 << mDirectory->getAbsPath(dirPath).toNative();
    }
  }
  return list;
}

// explicit template instantiations
template QStringList Library::searchForElements<ComponentCategory>() const
    noexcept;
template QStringList Library::searchForElements<PackageCategory>() const
    noexcept;
template QStringList Library::searchForElements<Symbol>() const noexcept;
template QStringList Library::searchForElements<Package>() const noexcept;
template QStringList Library::searchForElements<Component>() const noexcept;
template QStringList Library::searchForElements<Device>() const noexcept;

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Library::serialize(SExpression& root) const {
  LibraryBaseElement::serialize(root);
  root.ensureLineBreak();
  root.appendChild("url", mUrl);
  foreach (const Uuid& uuid, Toolbox::sortedQSet(mDependencies)) {
    root.ensureLineBreak();
    root.appendChild("dependency", uuid);
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
