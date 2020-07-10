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

#ifndef LIBREPCB_TRANSACTIONALDIRECTORY_H
#define LIBREPCB_TRANSACTIONALDIRECTORY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "filesystem.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/

namespace librepcb {

class TransactionalFileSystem;

/*******************************************************************************
 *  Class TransactionalDirectory
 ******************************************************************************/

/**
 * @brief Helper class to access a subdirectory of TransactionalFileSystem
 *
 * Wraps a subdirectory of a librepcb::TransactionalFileSystem instance to
 * allow accessing it like it was the root directory of a file system. In
 * addition, it allows to copy or move whole directories between different
 * transactional file systems.
 */
class TransactionalDirectory final : public FileSystem {
  Q_OBJECT

public:
  // Constructors / Destructor
  TransactionalDirectory(QObject* parent = nullptr);
  explicit TransactionalDirectory(std::shared_ptr<TransactionalFileSystem> fs,
                                  const QString& dir    = "",
                                  QObject*       parent = nullptr) noexcept;
  // Note: The "other" parameter must not be a const reference because of
  //       const correctness reasons! A const TransactionalDirectory must
  //       not be convertible to a non-const TransactionalDirectory because
  //       it would allow writing to a file system which should actually be
  //       const.
  TransactionalDirectory(TransactionalDirectory& other,
                         const QString&          subdir = "",
                         QObject*                parent = nullptr) noexcept;
  virtual ~TransactionalDirectory() noexcept;

  // Getters
  std::shared_ptr<const TransactionalFileSystem> getFileSystem() const
      noexcept {
    return mFileSystem;
  }
  std::shared_ptr<TransactionalFileSystem> getFileSystem() noexcept {
    return mFileSystem;
  }
  const QString& getPath() const noexcept { return mPath; }
  bool           isWritable() const noexcept;
  bool           isRestoredFromAutosave() const noexcept;

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
  void copyTo(TransactionalDirectory& dest) const;
  void saveTo(TransactionalDirectory& dest);
  void moveTo(TransactionalDirectory& dest);

private:  // Methods
  static void copyDirRecursively(TransactionalFileSystem& srcFs,
                                 const QString&           srcDir,
                                 TransactionalFileSystem& dstFs,
                                 const QString&           dstDir);

private:  // Data
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  QString                                  mPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_TRANSACTIONALDIRECTORY_H
