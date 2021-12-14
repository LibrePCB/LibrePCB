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
#include "bomcsvwriter.h"

#include "../fileio/csvfile.h"
#include "bom.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BomCsvWriter::BomCsvWriter(const Bom& bom) noexcept : mBom(bom) {
}

BomCsvWriter::~BomCsvWriter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<CsvFile> BomCsvWriter::generateCsv() const {
  std::shared_ptr<CsvFile> file(new CsvFile());

  // Don't translate the CSV header to make BOM files independent of the
  // user's language.
  file->setHeader(QStringList{"Quantity", "Designators"} + mBom.getColumns());

  foreach (const BomItem& item, mBom.getItems()) {
    QStringList values;
    values += QString::number(item.getDesignators().count());
    values += item.getDesignators().join(", ");
    foreach (const QString& attribute, item.getAttributes()) {
      values += attribute;
    }
    file->addValue(values);  // can throw
  }

  return file;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
