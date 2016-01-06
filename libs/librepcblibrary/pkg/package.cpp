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

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Package::Package(const Uuid& uuid, const Version& version, const QString& author,
                 const QString& name_en_US, const QString& description_en_US,
                 const QString& keywords_en_US) throw (Exception) :
    LibraryElement("pkg", "package", uuid, version, author, name_en_US, description_en_US, keywords_en_US),
    mDefaultFootprintUuid()
{
}

Package::Package(const FilePath& elementDirectory) throw (Exception) :
    LibraryElement(elementDirectory, "pkg", "package"),
    mDefaultFootprintUuid()
{
    try
    {
        readFromFile();
    }
    catch (Exception& e)
    {
        qDeleteAll(mFootprints);    mFootprints.clear();
        qDeleteAll(mPads);          mPads.clear();
        throw;
    }
}

Package::~Package() noexcept
{
    qDeleteAll(mFootprints);    mFootprints.clear();
    qDeleteAll(mPads);          mPads.clear();
}

/*****************************************************************************************
 *  PackagePad Methods
 ****************************************************************************************/

void Package::addPad(PackagePad& pad) noexcept
{
    Q_ASSERT(!mPads.contains(pad.getUuid()));
    // TODO: check if name is valid
    mPads.insert(pad.getUuid(), &pad);
}

void Package::removePad(PackagePad& pad) noexcept
{
    Q_ASSERT(mPads.contains(pad.getUuid()));
    Q_ASSERT(mPads.value(pad.getUuid()) == &pad);
    mPads.remove(pad.getUuid());
}

/*****************************************************************************************
 *  Footprint Methods
 ****************************************************************************************/

void Package::setDefaultFootprint(const Uuid& uuid) noexcept
{
    Q_ASSERT(mFootprints.contains(uuid));
    mDefaultFootprintUuid = uuid;
}

void Package::addFootprint(Footprint& footprint) noexcept
{
    Q_ASSERT(!mFootprints.contains(footprint.getUuid()));
    // TODO: check if name is valid
    mFootprints.insert(footprint.getUuid(), &footprint);
}

void Package::removeFootprint(Footprint& footprint) noexcept
{
    Q_ASSERT(mFootprints.contains(footprint.getUuid()));
    Q_ASSERT(mFootprints.value(footprint.getUuid()) == &footprint);
    mFootprints.remove(footprint.getUuid());
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
        mPads.insert(pad->getUuid(), pad);
    }

    // Load all footprints
    XmlDomElement* footprintsNode = root.getFirstChild("footprints", true);
    for (XmlDomElement* node = footprintsNode->getFirstChild("footprint", true);
         node; node = node->getNextSibling("footprint"))
    {
        Footprint* footprint = new Footprint(*node);
        if (getFootprintByUuid(footprint->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, footprint->getUuid().toStr(),
                QString(tr("The footprint \"%1\" exists multiple times in \"%2\"."))
                .arg(footprint->getUuid().toStr(), mXmlFilepath.toNative()));
        }
        mFootprints.insert(footprint->getUuid(), footprint);
    }

    // load default footprint
    mDefaultFootprintUuid = footprintsNode->getAttribute<Uuid>("default", true);
    if (!mFootprints.contains(mDefaultFootprintUuid)) {
        throw RuntimeError(__FILE__, __LINE__, mDefaultFootprintUuid.toStr(),
            QString(tr("The package \"%1\" has no valid default footprint set."))
            .arg(mXmlFilepath.toNative()));
    }
}

XmlDomElement* Package::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement());

    XmlDomElement* padsNode = root->appendChild("pads");
    foreach (const PackagePad* pad, mPads)
        padsNode->appendChild(pad->serializeToXmlDomElement());
    XmlDomElement* footprintsNode = root->appendChild("footprints");
    footprintsNode->setAttribute("default", mDefaultFootprintUuid);
    foreach (const Footprint* footprint, mFootprints) {
        footprintsNode->appendChild(footprint->serializeToXmlDomElement());
    }

    return root.take();
}

bool Package::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    if (mFootprints.isEmpty())                                  return false;
    if (mDefaultFootprintUuid.isNull())                         return false;
    if (!mFootprints.contains(mDefaultFootprintUuid))           return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
