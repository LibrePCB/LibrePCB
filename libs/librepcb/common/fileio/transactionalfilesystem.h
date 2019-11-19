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

#ifndef LIBREPCB_TRANSACTIONALFILESYSTEM_H
#define LIBREPCB_TRANSACTIONALFILESYSTEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "directorylock.h"
#include "filesystem.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/

class QuaZipFile;

namespace librepcb {

/*******************************************************************************
 *  Class TransactionalFileSystem
 ******************************************************************************/

/**
 * @brief Special FileSystem implementation for libraries and projects
 *
 * This is an implementation of the librepcb::FileSystem interface with many
 * features needed to create, open and save LibrePCB library elements and
 * projects in a very safe way to always guarantee consistency of all files.
 *
 * It handles following things:
 *  - Supports read-only access to the file system to guarantee absolutely
 *    nothing is written to the disk.
 *  - In R/W mode, it locks the accessed directory to avoid parallel usage (see
 *    @ref doc_project_lock)
 *  - Supports periodic saving to allow restoring the last autosave backup after
 *    an application crash (see @ref doc_project_autosave).
 *  - Holds all file modifications in memory and allows to write those in an
 *    atomic way to the disk (see @ref doc_project_save).
 *  - Allows to export the whole file system to a ZIP file.
 */
class TransactionalFileSystem final : public FileSystem {
  Q_OBJECT

public:
  enum class RestoreMode {
    NO,     ///< Do not restore the backup
    YES,    ///< Restore backup if available
    ASK,    ///< Ask (with QMessageBox) whether to restore or not
    ABORT,  ///< Throw a RuntimeError if a backup exists
  };

  // Constructors / Destructor
  TransactionalFileSystem() = delete;
  TransactionalFileSystem(const FilePath& filepath, bool writable = false,
                          RestoreMode restoreMode = RestoreMode::NO,
                          QObject*    parent      = nullptr);
  TransactionalFileSystem(const TransactionalFileSystem& other) = delete;
  virtual ~TransactionalFileSystem() noexcept;

  // Getters
  const FilePath& getPath() const noexcept { return mFilePath; }
  bool            isWritable() const noexcept { return mIsWritable; }
  bool isRestoredFromAutosave() const noexcept { return mRestoredFromAutosave; }

  // Inherited from FileSystem
  virtual FilePath getAbsPath(const QString& path = "") const noexcept override;
  virtual QStringList getDirs(const QString& path = "") const noexcept override;
  virtual QStringList getFiles(const QString& path = "") const
      noexcept override;
  virtual bool       fileExists(const QString& path) const noexcept override;
  virtual QByteArray read(const QString& path) const override;
  virtual void write(const QString& path, const QByteArray& content) override;
  virtual void removeFile(const QString& path) override;
  virtual void removeDirRecursively(const QString& path = "") override;

  // General Methods
  void        loadFromZip(const FilePath& fp);
  void        exportToZip(const FilePath& fp) const;
  void        discardChanges() noexcept;
  QStringList checkForModifications() const;
  void        autosave();
  void        save();

  // Static Methods
  static std::shared_ptr<TransactionalFileSystem> open(
      const FilePath& filepath, bool writable,
      RestoreMode restoreMode = RestoreMode::NO, QObject* parent = nullptr) {
    return std::make_shared<TransactionalFileSystem>(filepath, writable,
                                                     restoreMode, parent);
  }
  static std::shared_ptr<TransactionalFileSystem> openRO(
      const FilePath& filepath, RestoreMode restoreMode = RestoreMode::NO,
      QObject* parent = nullptr) {
    return open(filepath, false, restoreMode, parent);
  }
  static std::shared_ptr<TransactionalFileSystem> openRW(
      const FilePath& filepath, RestoreMode restoreMode = RestoreMode::NO,
      QObject* parent = nullptr) {
    return open(filepath, true, restoreMode, parent);
  }
  static QString cleanPath(QString path) noexcept;

private:  // Methods
  bool isRemoved(const QString& path) const noexcept;
  void exportDirToZip(QuaZipFile& file, const FilePath& zipFp,
                      const QString& dir) const;
  void saveDiff(const QString& type) const;
  void loadDiff(const FilePath& fp);
  void removeDiff(const QString& type);

private:  // Data
  FilePath      mFilePath;
  bool          mIsWritable;
  DirectoryLock mLock;
  bool          mRestoredFromAutosave;

  // File system modifications
  QHash<QString, QByteArray> mModifiedFiles;
  QSet<QString>              mRemovedFiles;
  QSet<QString>              mRemovedDirs;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_TRANSACTIONALFILESYSTEM_H
