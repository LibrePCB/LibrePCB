/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2020 LibrePCB Developers, see AUTHORS.md for contributors.
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

#ifndef LIBREPCB_PROJECT_LICENSES_SINGLEFILELICENSE_H
#define LIBREPCB_PROJECT_LICENSES_SINGLEFILELICENSE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "license_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Class SingleFileLicense
 ******************************************************************************/

/**
 * @brief This class wraps a license that can be applied by copying a single
 * file to `LICENSE.txt`.
 */
class SingleFileLicense : public LicenseBase {
public:
  // Constructors / Destructor
  SingleFileLicense(const QString sourceFilename,
                    const QString description) noexcept;

  // Getters
  const QList<std::pair<QString, QString>>& getFiles() noexcept {
    return mFiles;
  }
  const QString& getDescription() noexcept { return mDescription; }

private:  // Data
  QList<std::pair<QString, QString>> mFiles;
  QString                            mDescription;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_LICENSES_SINGLEFILELICENSE_H
