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
#include "smartsexprfile.h"

#include "fileutils.h"
#include "sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SmartSExprFile::SmartSExprFile(const FilePath& filepath, bool restore,
                               bool readOnly, bool create)
  : SmartFile(filepath, restore, readOnly, create) {
}

SmartSExprFile::~SmartSExprFile() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

SExpression SmartSExprFile::parseFileAndBuildDomTree() const {
  return SExpression::parse(FileUtils::readFile(mOpenedFilePath),
                            mOpenedFilePath);
}

void SmartSExprFile::save(const SExpression& domDocument, bool toOriginal) {
  FilePath filepath = prepareSaveAndReturnFilePath(toOriginal);  // can throw
  QString  content  = domDocument.toString(0);                   // can throw
  if (!content.endsWith('\n')) {
    content.append('\n');
  }
  FileUtils::writeFile(filepath, content.toUtf8());  // can throw
  updateMembersAfterSaving(toOriginal);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

SmartSExprFile* SmartSExprFile::create(const FilePath& filepath) {
  return new SmartSExprFile(filepath, false, false, true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
