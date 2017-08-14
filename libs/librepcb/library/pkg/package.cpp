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
#include "package.h"
#include <librepcb/common/fileio/domdocument.h>

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
                 const QString& keywords_en_US) :
    LibraryElement(getShortElementName(), getLongElementName(), uuid, version, author,
                   name_en_US, description_en_US, keywords_en_US)
{
}

Package::Package(const FilePath& elementDirectory, bool readOnly) :
    LibraryElement(elementDirectory, getShortElementName(), getLongElementName(), readOnly)
{
    try
    {
        const DomElement& root = mLoadingXmlFileDocument->getRoot();

        // Load all pads
        foreach (const DomElement* node, root.getChilds("pad")) {
            PackagePad* pad = new PackagePad(*node);
            if (getPadByUuid(pad->getUuid())) {
                throw RuntimeError(__FILE__, __LINE__,
                    QString(tr("The pad \"%1\" exists multiple times in \"%2\"."))
                    .arg(pad->getUuid().toStr(), root.getDocFilePath().toNative()));
            }
            mPads.insert(pad->getUuid(), pad);
        }

        // Load all footprints
        foreach (const DomElement* node, root.getChilds("footprint")) {
            Footprint* footprint = new Footprint(*node);
            if (getFootprintByUuid(footprint->getUuid())) {
                throw RuntimeError(__FILE__, __LINE__,
                    QString(tr("The footprint \"%1\" exists multiple times in \"%2\"."))
                    .arg(footprint->getUuid().toStr(), root.getDocFilePath().toNative()));
            }
            mFootprints.insert(footprint->getUuid(), footprint);
        }

        cleanupAfterLoadingElementFromFile();
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

void Package::serialize(DomElement& root) const
{
    LibraryElement::serialize(root);
    serializePointerContainer(root, mPads, "pad");
    serializePointerContainer(root, mFootprints, "footprint");
}

bool Package::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    if (mFootprints.isEmpty())                                  return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
