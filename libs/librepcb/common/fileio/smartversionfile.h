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

#ifndef LIBREPCB_SMARTVERSIONFILE_H
#define LIBREPCB_SMARTVERSIONFILE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../version.h"
#include "smartfile.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SmartVersionFile
 ******************************************************************************/

/**
 * @brief The SmartVersionFile class
 *
 * @note See class #SmartFile for more information.
 *
 * @author ubruhin
 * @date 2016-08-06
 */
class SmartVersionFile final : public SmartFile {
  Q_DECLARE_TR_FUNCTIONS(SmartVersionFile)

public:
  // Constructors / Destructor
  SmartVersionFile()                              = delete;
  SmartVersionFile(const SmartVersionFile& other) = delete;

  /**
   * @brief The constructor to open an existing version file
   *
   * This constructor tries to open an existing file and throws an exception if
   * an error occurs.
   *
   * @param filepath  See SmartFile#SmartFile()
   * @param restore   See SmartFile#SmartFile()
   * @param readOnly  See SmartFile#SmartFile()
   *
   * @throw Exception See SmartFile#SmartFile()
   */
  SmartVersionFile(const FilePath& filepath, bool restore, bool readOnly);

  /**
   * @copydoc SmartFile#~SmartFile()
   */
  ~SmartVersionFile() noexcept;

  // Getters

  /**
   * @brief Get the content of the file
   *
   * @return The content of the file
   */
  const Version& getVersion() const noexcept { return mVersion; }

  // Setters

  /**
   * @brief Set the version of the file
   *
   * @note The version won't be written to the file until #save() is called.
   *
   * @param version   The new version of the file
   */
  void setVersion(const Version& version) noexcept { mVersion = version; }

  // General Methods

  /**
   * @brief Write all changes to the file system
   *
   * @param toOriginal    Specifies whether the original or the backup file
   * should be overwritten/created.
   *
   * @throw Exception If an error occurs
   */
  void save(bool toOriginal);

  // Operator Overloadings
  SmartVersionFile& operator=(const SmartVersionFile& rhs) = delete;

  // Static Methods

  /**
   * @brief Create a new version file
   *
   * @note    This method will NOT immediately create the file! The file will be
   *          created after calling #save().
   *
   * @param filepath  The filepath to the file to create (always to the original
   * file, not to the backup file with "~" at the end of the filename!)
   *
   * @return The #SmartVersionFile object of the created file
   *
   * @throw Exception If an error occurs
   */
  static SmartVersionFile* create(const FilePath& filepath,
                                  const Version&  version);

protected:
  // Protected Methods

  /**
   * @brief Constructor to create a new version file
   *
   * @param filepath      See SmartFile#SmartFile()
   * @param newVersion    The new version of the file
   *
   * @throw Exception See SmartFile#SmartFile()
   */
  SmartVersionFile(const FilePath& filepath, const Version& newVersion);

  static Version readVersionFromFile(const FilePath& filepath);

  // General Attributes

  /**
   * @brief The version number of the file
   */
  Version mVersion;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SMARTVERSIONFILE_H
