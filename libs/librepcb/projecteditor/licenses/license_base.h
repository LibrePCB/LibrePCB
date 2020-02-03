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

#ifndef LIBREPCB_PROJECT_LICENSES_LICENSE_BASE_H
#define LIBREPCB_PROJECT_LICENSES_LICENSE_BASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Class LicenseBase
 ******************************************************************************/

/**
 * @brief The LicenseBase class used to apply licenses to a project.
 */
class LicenseBase {
public:
  virtual ~LicenseBase() noexcept {}

  /**
   * @brief Return reference to a list of (source-filename, target-filename)
   * pairs.
   */
  virtual const QList<std::pair<QString, QString>>& getFiles() = 0;

  /**
   * @brief Return the license description.
   */
  virtual const QString& getDescription() = 0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_LICENSES_LICENSE_BASE_H
