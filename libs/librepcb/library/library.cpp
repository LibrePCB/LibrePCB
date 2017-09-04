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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "library.h"
#include <librepcb/common/toolbox.h>
#include <librepcb/common/fileio/domdocument.h>
#include <librepcb/common/fileio/smartxmlfile.h>
#include "cat/componentcategory.h"
#include "cat/packagecategory.h"
#include "sym/symbol.h"
#include "pkg/package.h"
#include "cmp/component.h"
#include "dev/device.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Library::Library(const Uuid& uuid, const Version& version, const QString& author,
                 const QString& name_en_US, const QString& description_en_US,
                 const QString& keywords_en_US) :
    LibraryBaseElement(false, getShortElementName(), getLongElementName(), uuid, version,
                       author, name_en_US, description_en_US, keywords_en_US)
{
}

Library::Library(const FilePath& libDir, bool readOnly) :
    LibraryBaseElement(libDir, false, "lib", "library", readOnly)
{
    // check directory suffix
    if (libDir.getSuffix() != "lplib") {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The library directory does not have the suffix '.lplib':\n\n%1"))
            .arg(libDir.toNative()));
    }

    // read properties
    DomElement& root = mLoadingXmlFileDocument->getRoot();
    mUrl = QUrl(root.getFirstChild("url", true)->getText<QString>(false), QUrl::StrictMode);

    // read dependency UUIDs
    foreach (const DomElement* node, root.getChilds("dependency")) {
        mDependencies.insert(node->getText<Uuid>(true));
    }

    // load image if available
    if (getIconFilePath().isExistingFile()) {
        mIcon = QPixmap(getIconFilePath().toStr());
    }

    cleanupAfterLoadingElementFromFile();
}

Library::~Library() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

template <typename ElementType>
FilePath Library::getElementsDirectory() const noexcept
{
    return mDirectory.getPathTo(ElementType::getShortElementName());
}

// explicit template instantiations
template FilePath Library::getElementsDirectory<ComponentCategory>() const noexcept;
template FilePath Library::getElementsDirectory<PackageCategory>() const noexcept;
template FilePath Library::getElementsDirectory<Symbol>() const noexcept;
template FilePath Library::getElementsDirectory<Package>() const noexcept;
template FilePath Library::getElementsDirectory<Component>() const noexcept;
template FilePath Library::getElementsDirectory<Device>() const noexcept;

FilePath Library::getIconFilePath() const noexcept
{
    return mDirectory.getPathTo("library.png");
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Library::setIconFilePath(const FilePath& png) noexcept
{
    if (png == getIconFilePath()) {
        return;
    }

    if (getIconFilePath().isExistingFile()) {
        QFile(getIconFilePath().toStr()).remove();
    }

    if (png.isExistingFile()) {
        QFile::copy(png.toStr(), getIconFilePath().toStr());
        mIcon = QPixmap(getIconFilePath().toStr());
    } else {
        mIcon = QPixmap();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Library::addDependency(const Uuid& uuid) noexcept
{
    if ((!uuid.isNull()) && (!mDependencies.contains(uuid))) {
        mDependencies.insert(uuid);
    } else {
        qWarning() << "Invalid or duplicate library dependency:" << uuid.toStr();
    }
}

void Library::removeDependency(const Uuid& uuid) noexcept
{
    if ((!uuid.isNull()) && (mDependencies.contains(uuid))) {
        mDependencies.remove(uuid);
    } else {
        qWarning() << "Invalid library dependency:" << uuid.toStr();
    }
}

template <typename ElementType>
QList<FilePath> Library::searchForElements() const noexcept
{
    QList<FilePath> list;
    FilePath subDirFilePath = getElementsDirectory<ElementType>();
    QDir subDir(subDirFilePath.toStr());
    foreach (const QString& dirname, subDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        FilePath elementFilePath = subDirFilePath.getPathTo(dirname);
        if (isValidElementDirectory<ElementType>(elementFilePath)) {
            list.append(elementFilePath);
        } else {
            qWarning() << "Directory is not a valid library element:" << elementFilePath.toNative();
        }
    }
    return list;
}

// explicit template instantiations
template QList<FilePath> Library::searchForElements<ComponentCategory>() const noexcept;
template QList<FilePath> Library::searchForElements<PackageCategory>() const noexcept;
template QList<FilePath> Library::searchForElements<Symbol>() const noexcept;
template QList<FilePath> Library::searchForElements<Package>() const noexcept;
template QList<FilePath> Library::searchForElements<Component>() const noexcept;
template QList<FilePath> Library::searchForElements<Device>() const noexcept;

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Library::copyTo(const FilePath& destination, bool removeSource)
{
    // check directory suffix
    if (destination.getSuffix() != "lplib") {
        qDebug() << destination.toStr();
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("A library directory name must have the suffix '.lplib'.")));
    }

    // copy the element
    LibraryBaseElement::copyTo(destination, removeSource);
}

void Library::serialize(DomElement& root) const
{
    LibraryBaseElement::serialize(root);
    root.appendTextChild("url", mUrl.toString(QUrl::PrettyDecoded));
    foreach (const Uuid& uuid, Toolbox::sortedQSet(mDependencies)) {
        root.appendTextChild("dependency", uuid);
    }
}

bool Library::checkAttributesValidity() const noexcept
{
    if (!LibraryBaseElement::checkAttributesValidity()) return false;
    foreach (const Uuid& uuid, mDependencies) {
        if (uuid.isNull()) return false;
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
