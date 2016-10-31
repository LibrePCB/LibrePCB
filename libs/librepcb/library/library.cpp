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
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/smartxmlfile.h>
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
                 const QString& keywords_en_US) throw (Exception) :
    LibraryBaseElement(false, getShortElementName(), getLongElementName(), uuid, version,
                       author, name_en_US, description_en_US, keywords_en_US)
{
}

Library::Library(const FilePath& libDir, bool readOnly) throw (Exception) :
    LibraryBaseElement(libDir, false, "lib", "library", readOnly)
{
    // check directory suffix
    if (libDir.getSuffix() != "lplib") {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The library directory does not have the suffix '.lplib':\n\n%1"))
            .arg(libDir.toNative()));
    }

    // read properties
    XmlDomElement& root = mLoadingXmlFileDocument->getRoot();
    mUrl = QUrl(root.getFirstChild("properties/url", true, true)->getText<QString>(false), QUrl::StrictMode);

    // read dependency UUIDs
    for (XmlDomElement* node = root.getFirstChild("properties/dependency", true, false);
         node; node = node->getNextSibling("dependency"))
    {
        mDependencies.append(node->getText<Uuid>(true));
    }

    // load image if available
    FilePath iconFilePath = libDir.getPathTo("icon.png");
    if (iconFilePath.isExistingFile()) {
        mIcon = QPixmap(iconFilePath.toStr());
    }

    cleanupAfterLoadingElementFromFile();
}

Library::~Library() noexcept
{
}


/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Library::addDependency(const Uuid& uuid) noexcept
{
    if ((!uuid.isNull()) && (!mDependencies.contains(uuid))) {
        mDependencies.append(uuid);
    } else {
        qWarning() << "Invalid or duplicate library dependency:" << uuid.toStr();
    }
}

void Library::removeDependency(const Uuid& uuid) noexcept
{
    if ((!uuid.isNull()) && (mDependencies.contains(uuid))) {
        mDependencies.removeAll(uuid);
    } else {
        qWarning() << "Invalid library dependency:" << uuid.toStr();
    }
}

template <typename ElementType>
QList<FilePath> Library::searchForElements() const noexcept
{
    QList<FilePath> list;
    FilePath subDirFilePath = mDirectory.getPathTo(ElementType::getShortElementName());
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

void Library::copyTo(const FilePath& destination, bool removeSource) throw (Exception)
{
    // check directory suffix
    if (destination.getSuffix() != "lplib") {
        throw RuntimeError(__FILE__, __LINE__, destination.toNative(),
            QString(tr("A library directory name must have the suffix '.lplib'.")));
    }

    // copy the element
    LibraryBaseElement::copyTo(destination, removeSource);
}

XmlDomElement* Library::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryBaseElement::serializeToXmlDomElement());
    XmlDomElement* properties = root->appendChild("properties");
    properties->appendTextChild("url", mUrl.toString(QUrl::PrettyDecoded));
    foreach (const Uuid& uuid, mDependencies) {
        properties->appendTextChild("dependency", uuid);
    }
    return root.take();
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
