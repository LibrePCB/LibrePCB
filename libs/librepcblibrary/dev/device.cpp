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
#include "device.h"
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Device::Device(const Uuid& uuid, const Version& version, const QString& author,
               const QString& name_en_US, const QString& description_en_US,
               const QString& keywords_en_US) throw (Exception) :
    LibraryElement("dev", "device", uuid, version, author, name_en_US,
                   description_en_US, keywords_en_US)
{
}

Device::Device(const FilePath& elementDirectory, bool readOnly) throw (Exception) :
    LibraryElement(elementDirectory, "dev", "device", readOnly)
{
    XmlDomElement& root = mLoadingXmlFileDocument->getRoot();

    // load attributes
    mComponentUuid = root.getFirstChild("meta/component", true, true)->getText<Uuid>(true);
    mPackageUuid = root.getFirstChild("meta/package", true, true)->getText<Uuid>(true);
    for (XmlDomElement* node = root.getFirstChild("pad_signal_map/map", true, false);
         node; node = node->getNextSibling("map"))
    {
        Uuid pad = node->getAttribute<Uuid>("pad", true);
        Uuid signal = node->getText<Uuid>(false);
        if (mPadSignalMap.contains(pad)) {
            throw RuntimeError(__FILE__, __LINE__, pad.toStr(),
                QString(tr("The pad \"%1\" exists multiple times in \"%2\"."))
                .arg(pad.toStr(), root.getDocFilePath().toNative()));
        }
        mPadSignalMap.insert(pad, signal);
    }

    cleanupAfterLoadingElementFromFile();
}

Device::~Device() noexcept
{
}

/*****************************************************************************************
 *  Pad-Signal-Map Methods
 ****************************************************************************************/

void Device::addPadSignalMapping(const Uuid& pad, const Uuid& signal) noexcept
{
    Q_ASSERT(!mPadSignalMap.contains(pad));
    mPadSignalMap.insert(pad, signal);
}

void Device::removePadSignalMapping(const Uuid& pad) noexcept
{
    Q_ASSERT(mPadSignalMap.contains(pad));
    mPadSignalMap.remove(pad);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

XmlDomElement* Device::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement());
    root->getFirstChild("meta", true)->appendTextChild("component", mComponentUuid);
    root->getFirstChild("meta", true)->appendTextChild("package", mPackageUuid);
    XmlDomElement* padSignalMap = root->appendChild("pad_signal_map");
    foreach (const Uuid& padUuid, mPadSignalMap.keys()) {
        XmlDomElement* child = padSignalMap->appendChild("map");
        child->setAttribute("pad", padUuid);
        child->setText(mPadSignalMap.value(padUuid));
    }
    return root.take();
}

bool Device::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    if (mComponentUuid.isNull())                                return false;
    if (mPackageUuid.isNull())                                  return false;
    foreach (const Uuid& padUuid, mPadSignalMap.keys()) {
        if (padUuid.isNull())                                   return false;
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
