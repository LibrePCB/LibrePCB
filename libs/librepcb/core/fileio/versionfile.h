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

#ifndef LIBREPCB_CORE_VERSIONFILE_H
#define LIBREPCB_CORE_VERSIONFILE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/version.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class VersionFile
 ******************************************************************************/

/**
 * @brief Class for reading and writing version files from/to QByteArray
 *
 * See @ref doc_versioning for details what version files are used for and how
 * they work.
 *
 * @see @ref doc_versioning
 */
class VersionFile final {
  Q_DECLARE_TR_FUNCTIONS(VersionFile)

public:
  // Constructors / Destructor
  VersionFile() = delete;
  VersionFile(const VersionFile& other) = default;

  /**
   * @brief The constructor to create a new version file
   *
   * @param version   The file version
   */
  VersionFile(const Version& version) noexcept;

  /**
   * Destructor
   */
  ~VersionFile() noexcept;

  // Getters

  /**
   * @brief Get the content of the file
   *
   * @return The version contained in the file
   */
  const Version& getVersion() const noexcept { return mVersion; }

  // Setters

  /**
   * @brief Set the version of the file
   *
   * @param version   The new version of the file
   */
  void setVersion(const Version& version) noexcept { mVersion = version; }

  // General Methods

  /**
   * @brief Export file content as byte array
   */
  QByteArray toByteArray() const noexcept;

  /**
   * @brief Load version file from byte array
   *
   * @param content   The raw file content
   *
   * @return A new VersionFile object
   *
   * @throw Exception if the content is invalid
   */
  static VersionFile fromByteArray(const QByteArray& content);

  // Operator Overloadings
  VersionFile& operator=(const VersionFile& rhs) = delete;

private:  // Data
  /**
   * @brief The version number of the file
   */
  Version mVersion;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
