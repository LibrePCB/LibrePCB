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
#include <librepcb/common/fileio/sexpression.h>

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
               const QString& keywords_en_US, const Uuid& component, const Uuid& package) :
    LibraryElement(getShortElementName(), getLongElementName(), uuid, version, author,
                   name_en_US, description_en_US, keywords_en_US),
    mComponentUuid(component), mPackageUuid(package), mAttributes(), mPadSignalMap()
{
}

Device::Device(const FilePath& elementDirectory, bool readOnly) :
    LibraryElement(elementDirectory, getShortElementName(), getLongElementName(), readOnly),
    mComponentUuid(mLoadingFileDocument.getValueByPath<Uuid>("component")),
    mPackageUuid(mLoadingFileDocument.getValueByPath<Uuid>("package")),
    mAttributes(mLoadingFileDocument),
    mPadSignalMap(mLoadingFileDocument)
{
    cleanupAfterLoadingElementFromFile();
}

Device::~Device() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Device::setComponentUuid(const Uuid& uuid) noexcept
{
    if (uuid == mComponentUuid) return;
    mComponentUuid = uuid;
    emit componentUuidChanged(mComponentUuid);
}

void Device::setPackageUuid(const Uuid& uuid) noexcept
{
    if (uuid == mPackageUuid) return;
    mPackageUuid = uuid;
    emit packageUuidChanged(mPackageUuid);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Device::serialize(SExpression& root) const
{
    LibraryElement::serialize(root);
    root.appendChild("component", mComponentUuid, true);
    root.appendChild("package", mPackageUuid, true);
    mAttributes.serialize(root);
    mPadSignalMap.sortedByUuid().serialize(root);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
