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
#include "smartversionfile.h"

#include "fileutils.h"
#include "versionfile.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SmartVersionFile::SmartVersionFile(const FilePath& filepath, bool restore,
                                   bool readOnly)
  : SmartFile(filepath, restore, readOnly, false),
    mVersionFile(readVersionFromFile(mOpenedFilePath)) {
}

SmartVersionFile::SmartVersionFile(const FilePath& filepath,
                                   const Version&  newVersion)
  : SmartFile(filepath, false, false, true), mVersionFile(newVersion) {
}

SmartVersionFile::~SmartVersionFile() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SmartVersionFile::save(bool toOriginal) {
  const FilePath& filepath = prepareSaveAndReturnFilePath(toOriginal);
  FileUtils::writeFile(filepath, mVersionFile.toByteArray());
  updateMembersAfterSaving(toOriginal);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

SmartVersionFile* SmartVersionFile::create(const FilePath& filepath,
                                           const Version&  version) {
  return new SmartVersionFile(filepath, version);
}

VersionFile SmartVersionFile::readVersionFromFile(const FilePath& filepath) {
  try {
    return VersionFile::fromByteArray(
        FileUtils::readFile(filepath));  // can throw
  } catch (const Exception& e) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Could not read version number from \"%1\": %2"))
            .arg(filepath.toNative(), e.getMsg()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
