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

#include "../fileio/fileutils.h"
#include "bom.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BomCsvWriter::BomCsvWriter() noexcept {
}

BomCsvWriter::~BomCsvWriter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QList<QStringList> BomCsvWriter::toStringList(const Bom& bom) noexcept {
  QList<QStringList> lines;
  // Don't translate the CSV header to make BOMs independent of the user's
  // language.
  lines.append(QStringList{"Quantity", "Designators"} + bom.getColumns());
  foreach (const BomItem& item, bom.getItems()) {
    QStringList cols;
    cols += QString::number(item.getDesignators().count());
    cols += cleanStr(item.getDesignators().join(", "));
    foreach (const QString& attribute, item.getAttributes()) {
      cols += cleanStr(attribute);
    }
    lines.append(cols);
  }
  return lines;
}

QString BomCsvWriter::toString(const Bom& bom) noexcept {
  QString str;
  foreach (const QStringList& line, toStringList(bom)) {
    str += line.join(";") + "\n";
  }
  return str;
}

void BomCsvWriter::writeToFile(const Bom& bom, const FilePath& csvFp) {
  FileUtils::writeFile(csvFp, toString(bom).toUtf8());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString BomCsvWriter::cleanStr(const QString& str) noexcept {
  QString cleaned = str;
  cleaned.replace(";", " ");   // semicolon is reserved for separators
  cleaned.replace("\n", " ");  // BOM rows shouldn't be multiline
  return cleaned.trimmed();    // remove leading and trailing whitespaces
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
