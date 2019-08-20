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

#ifndef LIBREPCB_BOMCSVWRITER_H
#define LIBREPCB_BOMCSVWRITER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Bom;
class FilePath;

/*******************************************************************************
 *  Class BomCsvWriter
 ******************************************************************************/

/**
 * @brief The BomCsvWriter class
 */
class BomCsvWriter final {
  Q_DECLARE_TR_FUNCTIONS(BomCsvWriter)

public:
  // Constructors / Destructor
  BomCsvWriter() noexcept;
  BomCsvWriter(const BomCsvWriter& other) = delete;
  ~BomCsvWriter() noexcept;

  // General Methods
  QList<QStringList> toStringList(const Bom& bom) noexcept;
  QString            toString(const Bom& bom) noexcept;
  void               writeToFile(const Bom& bom, const FilePath& csvFp);

  // Operator Overloadings
  BomCsvWriter& operator=(const BomCsvWriter& rhs) = delete;

private:
  static QString cleanStr(const QString& str) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_BOMCSVWRITER_H
