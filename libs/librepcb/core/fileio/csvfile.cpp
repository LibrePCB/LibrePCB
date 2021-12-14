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
#include "csvfile.h"

#include "../fileio/fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CsvFile::CsvFile() noexcept : mComment(), mHeader(), mValues() {
}

CsvFile::~CsvFile() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void CsvFile::setComment(const QString& comment) noexcept {
  mComment = comment;
}

void CsvFile::setHeader(const QStringList& header) noexcept {
  mHeader = header;
  mValues.clear();  // column count may have changed -> clear all values!
}

void CsvFile::addValue(const QStringList& value) {
  if (value.count() != mHeader.count()) {
    throw LogicError(__FILE__, __LINE__,
                     "CSV value count is different to header item count.");
  }
  mValues.append(value);
}

QString CsvFile::toString() const noexcept {
  QString str = getCommentLines();
  str += lineToString(mHeader);
  foreach (const QStringList& value, mValues) { str += lineToString(value); }
  return str;
}

void CsvFile::saveToFile(const FilePath& csvFp) const {
  FileUtils::writeFile(csvFp, toString().toUtf8());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString CsvFile::getCommentLines() const noexcept {
  QString str;
  if (!mComment.isEmpty()) {
    foreach (QString line, mComment.split("\n", QString::KeepEmptyParts)) {
      str += "# " % line;
      while (str[str.count() - 1].isSpace()) {
        str.chop(1);
      }
      str += "\n";
    }
    str += "\n";  // separate comment and CSV data with an empty line
  }
  return str;
}

QString CsvFile::lineToString(const QStringList& line) const noexcept {
  QString str;
  // Note: To guarantee equal value count on each line, we always use the
  //       header to determine the value count. If a line contains more values,
  //       they are ignored. If a line contains less values, empty strings will
  //       be used instead.
  for (int i = 0; i < mHeader.count(); ++i) {
    str += escapeValue(line.value(i));
    if (i < mHeader.count() - 1) {
      str += ",";
    } else {
      str += "\n";
    }
  }
  return str;
}

QString CsvFile::escapeValue(const QString& value) noexcept {
  QString escaped = value;
  escaped.remove("\r");  // remove DOS line endings, if any
  escaped.replace("\n", " ");  // replace linebreaks by spaces
  if (escaped.contains(",") || escaped.contains("\"")) {
    escaped.replace("\"", "\"\"");  // escape quotes
    escaped = "\"" + escaped + "\"";  // add quotes around value
  }
  return escaped;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
