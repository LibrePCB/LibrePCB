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
#include "smartxmlfile.h"
#include "fileutils.h"
#include "domdocument.h"
#include "domelement.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartXmlFile::SmartXmlFile(const FilePath& filepath, bool restore, bool readOnly, bool create) :
    SmartFile(filepath, restore, readOnly, create)
{
}

SmartXmlFile::~SmartXmlFile() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

std::unique_ptr<DomDocument> SmartXmlFile::parseFileAndBuildDomTree() const
{
    return std::unique_ptr<DomDocument>(
        new DomDocument(FileUtils::readFile(mOpenedFilePath), mOpenedFilePath));
}

void SmartXmlFile::save(const DomDocument& domDocument, bool toOriginal)
{
    const FilePath& filepath = prepareSaveAndReturnFilePath(toOriginal);
    FileUtils::writeFile(filepath, domDocument.toByteArray());
    updateMembersAfterSaving(toOriginal);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SmartXmlFile* SmartXmlFile::create(const FilePath &filepath)
{
    return new SmartXmlFile(filepath, false, false, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
