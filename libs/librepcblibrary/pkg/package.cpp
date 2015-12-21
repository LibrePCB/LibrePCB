/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "package.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Package::Package(const Uuid& uuid, const Version& version, const QString& author,
                 const QString& name_en_US, const QString& description_en_US,
                 const QString& keywords_en_US) throw (Exception) :
    LibraryElement("pkg", "package", uuid, version, author, name_en_US, description_en_US, keywords_en_US)
{
}

Package::Package(const FilePath& elementDirectory) throw (Exception) :
    LibraryElement(elementDirectory, "pkg", "package")
{
    readFromFile();
}

Package::~Package() noexcept
{
    clearPads();
    clearFootprints();
}

/*****************************************************************************************
 *  Pads
 ****************************************************************************************/

const PackagePad* Package::getPadByUuid(const Uuid& uuid) const noexcept
{
    foreach (const PackagePad* pad, mPads)
    {
        if (pad->getUuid() == uuid)
            return pad;
    }
    return nullptr;
}

void Package::clearPads() noexcept
{
    qDeleteAll(mPads);
    mPads.clear();
}

void Package::addPad(const PackagePad& pad) noexcept
{
    Q_ASSERT(getPadByUuid(pad.getUuid()) == nullptr);
    mPads.append(&pad);
}

/*****************************************************************************************
 *  Footprints
 ****************************************************************************************/

const Footprint* Package::getFootprintByUuid(const Uuid& uuid) const noexcept
{
    foreach (const Footprint* footprint, mFootprints)
    {
        if (footprint->getUuid() == uuid)
            return footprint;
    }
    return nullptr;
}

void Package::clearFootprints() noexcept
{
    qDeleteAll(mFootprints);
    mFootprints.clear();
}

void Package::addFootprint(const Footprint& footprint) noexcept
{
    Q_ASSERT(getFootprintByUuid(footprint.getUuid()) == nullptr);
    mFootprints.append(&footprint);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Package::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryElement::parseDomTree(root);

    // Load all pads
    for (XmlDomElement* node = root.getFirstChild("pads/pad", true, false);
         node; node = node->getNextSibling("pad"))
    {
        PackagePad* pad = new PackagePad(*node);
        if (getPadByUuid(pad->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, pad->getUuid().toStr(),
                QString(tr("The pad \"%1\" exists multiple times in \"%2\"."))
                .arg(pad->getUuid().toStr(), mXmlFilepath.toNative()));
        }
        mPads.append(pad);
    }

    // Load all footprints
    for (XmlDomElement* node = root.getFirstChild("footprints/footprint", true, true);
         node; node = node->getNextSibling("footprint"))
    {
        Footprint* footprint = new Footprint(*node);
        if (getFootprintByUuid(footprint->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, footprint->getUuid().toStr(),
                QString(tr("The footprint \"%1\" exists multiple times in \"%2\"."))
                .arg(footprint->getUuid().toStr(), mXmlFilepath.toNative()));
        }
        mFootprints.append(footprint);
    }
}

XmlDomElement* Package::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement());

    XmlDomElement* padsNode = root->appendChild("pads");
    foreach (const PackagePad* pad, mPads)
        padsNode->appendChild(pad->serializeToXmlDomElement());
    XmlDomElement* footprintsNode = root->appendChild("footprints");
    foreach (const Footprint* footprint, mFootprints)
        footprintsNode->appendChild(footprint->serializeToXmlDomElement());

    return root.take();
}

bool Package::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
