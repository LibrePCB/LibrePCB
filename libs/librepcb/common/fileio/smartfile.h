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

#ifndef LIBREPCB_SMARTFILE_H
#define LIBREPCB_SMARTFILE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "filepath.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SmartFile
 ******************************************************************************/

/**
 * @brief   The abstract SmartFile class represents a file and provides some
 * useful methods to work with that file
 *
 * Features:
 *  - Open files in read-only mode (this class then guarantees that no write
 * operations are possible to that file)
 *  - Creation of backup files ('~' at the end of the filename)
 *  - Restoring backup files
 *  - Helper methods for subclasses to load/save files
 *
 * @note See @ref doc_project_save for more details about the backup/restore
 * feature.
 *
 * @author ubruhin
 * @date 2015-01-25
 *
 * @todo Test the class #SmartFile and all of its subclasses!
 */
class SmartFile {
  Q_DECLARE_TR_FUNCTIONS(SmartFile)

public:
  // Constructors / Destructor
  SmartFile()                       = delete;
  SmartFile(const SmartFile& other) = delete;

  /**
   * @brief The constructor
   *
   * @param filepath  The filepath to the file (always to a original file, not
   * to a backup file with "~" at the end of the filename!)
   * @param restore   If true and a backup (*~) of the specified file exists,
   * the backup will be opened instead of the original file. If no backup exists
   * or this parameter is false, the original file will be opened.
   * @param readOnly  If true, the file will be opened read-only (see
   * #mIsReadOnly)
   * @param create    If true, the file will be created/overwritten after saving
   *                  it the first time.
   *
   * @throw Exception If the specified file does not exist, an exception will be
   * thrown.
   */
  SmartFile(const FilePath& filepath, bool restore, bool readOnly, bool create);

  /**
   * @brief The destructor
   *
   * Removes the temporary file (if existing)
   */
  virtual ~SmartFile() noexcept;

  // Getters

  /**
   * @brief Get the filepath to the file which was passed to the constructor
   *
   * @return The filepath to the file
   */
  const FilePath& getFilepath() const noexcept { return mFilePath; }

  /**
   * @brief Check if this file was restored from a backup
   *
   * @return true if restored, false if not
   *
   * @see #mIsRestored
   */
  bool isRestored() const noexcept { return mIsRestored; }

  /**
   * @brief Check if this file was opened in read-only mode
   *
   * @return true if opened read-only, false if not
   *
   * @see #mIsReadOnly
   */
  bool isReadOnly() const noexcept { return mIsReadOnly; }

  /**
   * @brief Check if this file is created and not yet saved to the harddisc
   *
   * @return true if the file was not yet written the the harddisc, false
   * otherwise
   *
   * @see #mIsCreated
   */
  bool isCreated() const noexcept { return mIsCreated; }

  // General Methods

  /**
   * @brief Remove the file from the file system
   *
   * @param original  Specifies whether the original or the backup file should
   * be removed.
   *
   * @throw Exception If an error occurs, an exception will be thrown
   */
  void removeFile(bool original);

  // Operator Overloadings
  SmartFile& operator=(const SmartFile& rhs) = delete;

protected:
  // Protected Methods

  /**
   * @brief Prepare to save the file and return the filepath to the file
   *
   * This method:
   *  - throws an exception if the file was opened in read-only mode
   *  - tries to create all parent directories of the file to save
   *
   * @note This method must be called from all subclasses BEFORE saving the
   * changes to the file!
   *
   * @param toOriginal    Specifies whether the original or the backup file
   * should be overwritten/created. The path to that file will be returned
   * afterwards.
   *
   * @return The filepath to the file to save (either original or backup file)
   *
   * @throw Exception If an error occurs
   */
  const FilePath& prepareSaveAndReturnFilePath(bool toOriginal);

  /**
   * @brief Update the member variables #mIsRestored and #mIsCreated after
   * saving
   *
   * @note This method must be called from all subclasses AFTER saving the
   * changes to the file!
   *
   * @param toOriginal    Specifies whether the original or the backup file was
   * saved.
   */
  void updateMembersAfterSaving(bool toOriginal) noexcept;

  // General Attributes

  /**
   * @brief The filepath which was passed to the constructor
   */
  FilePath mFilePath;

  /**
   * @brief The filepath to the temporary file (#mFilePath + '~')
   */
  FilePath mTmpFilePath;

  /**
   * @brief The filepath from where the content was loaded
   *
   * If the backup file was loaded, this equals to #mFilePath with appended
   * tilde ('~'). If the original file was loaded, this equals to #mFilePath.
   */
  FilePath mOpenedFilePath;

  /**
   * @brief This variable determines whether the file was restored or not
   *
   * This file is set to true when the constructor was called with the parameter
   * "restore == true". After calling #updateMembersAfterSaving() with parameter
   * "toOriginal == true", this flag will be reset to false. The destructor
   * needs this flag to decide whether the temporary file should be removed or
   * not.
   */
  bool mIsRestored;

  /**
   * @brief If true, the file is opened as read-only
   *
   * @li No temporary files will be created/removed
   * @li #prepareSaveAndReturnFilePath() will always throw an exception
   */
  bool mIsReadOnly;

  /**
   * @brief If true, the file was created and not yet written to the filesystem
   * (so the file #mFilePath does not yet exist!)
   */
  bool mIsCreated;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SMARTFILE_H
