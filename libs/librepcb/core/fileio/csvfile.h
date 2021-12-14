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

#ifndef LIBREPCB_CORE_CSVFILE_H
#define LIBREPCB_CORE_CSVFILE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

/*******************************************************************************
 *  Class CsvFile
 ******************************************************************************/

/**
 * @brief The CsvFile class represents a comma-separated values (CSV) file
 *
 * The class allows building CSV and write them to a file. It is guaranteed
 * that the written files are valid:
 *
 *  - Whenn adding a row with a wrong value count, #addValue() throws an
 *    exception.
 *  - Linebreaks inside values are replaced by spaces.
 *  - If a value contains the separator character (e.g. the comma), the value
 *    gets quoted.
 *  - Quotes inside values are escaped.
 *
 * @note You have to call #setHeader() *before* adding any values with
 *       #addValue()! This is needed to make sure all value rows have the same
 *       value count as the header.
 *
 * @see https://en.wikipedia.org/wiki/Comma-separated_values
 */
class CsvFile final {
  Q_DECLARE_TR_FUNCTIONS(CsvFile)

public:
  // Constructors / Destructor
  CsvFile() noexcept;
  CsvFile(const CsvFile& other) = delete;
  ~CsvFile() noexcept;

  /**
   * @brief Get the comment of the file
   *
   * @return File comment (raw comment without '#' at beginning of lines).
   */
  const QString& getComment() const noexcept { return mComment; }

  /**
   * @brief Get the CSV header items
   *
   * @return CSV header items (raw, i.e. without quotes and escaped characters).
   */
  const QStringList& getHeader() const noexcept { return mHeader; }

  /**
   * @brief Get the CSV values
   *
   * @return All value rows (raw, i.e. without quotes and escaped characters).
   */
  const QList<QStringList>& getValues() const noexcept { return mValues; }

  /**
   * @brief Set file comment
   *
   * @param comment   The comment to set. May contain linebreaks.
   */
  void setComment(const QString& comment) noexcept;

  /**
   * @brief Set the header items
   *
   * @param header  The new header items.
   *
   * @warning This method clears all values!
   */
  void setHeader(const QStringList& header) noexcept;

  /**
   * @brief Add a row of values
   *
   * @param value   The value row items.
   *
   * @throw ::librepcb::Exception if the value item count is different to the
   *        header item count.
   */
  void addValue(const QStringList& value);

  /**
   * @brief Build CSV file content and return it as a string
   *
   * @return The string with the whole CSV file content.
   */
  QString toString() const noexcept;

  /**
   * @brief Write CSV file content to a file
   *
   * @param csvFp   The destination file path.
   *
   * @throw ::librepcb::Exception if the file could not be written.
   */
  void saveToFile(const FilePath& csvFp) const;

  // Operator Overloadings
  CsvFile& operator=(const CsvFile& rhs) = delete;

private:  // Methods
  QString getCommentLines() const noexcept;
  QString lineToString(const QStringList& line) const noexcept;
  static QString escapeValue(const QString& value) noexcept;

private:  // Data
  QString mComment;
  QStringList mHeader;
  QList<QStringList> mValues;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
