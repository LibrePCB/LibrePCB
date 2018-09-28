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

#ifndef LIBREPCB_EAGLEIMPORT_CONVERTERDB_H
#define LIBREPCB_EAGLEIMPORT_CONVERTERDB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

namespace eagleimport {

/*******************************************************************************
 *  Class ConverterDb
 ******************************************************************************/

/**
 * @brief The ConverterDb class
 */
class ConverterDb final {
public:
  // Constructors / Destructor
  ConverterDb()                         = delete;
  ConverterDb(const ConverterDb& other) = delete;
  ConverterDb(const FilePath& ini) noexcept;
  ~ConverterDb() noexcept;

  // General Methods
  void setCurrentLibraryFilePath(const FilePath& fp) noexcept {
    mLibFilePath = fp;
  }
  const FilePath& getCurrentLibraryFilePath() const noexcept {
    return mLibFilePath;
  }
  Uuid getSymbolUuid(const QString& symbolName);
  Uuid getSymbolPinUuid(const Uuid& symbolUuid, const QString& pinName);
  Uuid getFootprintUuid(const QString& packageName);
  Uuid getPackageUuid(const QString& packageName);
  Uuid getPackagePadUuid(const Uuid& footprintUuid, const QString& padName);
  Uuid getComponentUuid(const QString& deviceSetName);
  Uuid getComponentSignalUuid(const Uuid&    componentUuid,
                              const QString& gateName, const QString& pinName);
  Uuid getSymbolVariantUuid(const Uuid& componentUuid);
  Uuid getSymbolVariantItemUuid(const Uuid&    componentUuid,
                                const QString& gateName);
  Uuid getDeviceUuid(const QString& deviceSetName, const QString& deviceName);

  // Operator Overloadings
  ConverterDb& operator=(const ConverterDb& rhs) = delete;

private:
  Uuid getOrCreateUuid(const QString& cat, const QString& key1,
                       const QString& key2 = QString());

  QSettings mIniFile;
  FilePath  mLibFilePath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif  // LIBREPCB_EAGLEIMPORT_CONVERTERDB_H
