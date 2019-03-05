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

#ifndef LIBREPCB_FILESYSTEM_H
#define LIBREPCB_FILESYSTEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "filepath.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/

namespace librepcb {

/*******************************************************************************
 *  Class FileSystem
 ******************************************************************************/

/**
 * @brief Base class / interface for all file system implementations
 */
class FileSystem : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  FileSystem(QObject* parent = nullptr) noexcept : QObject(parent) {}
  virtual ~FileSystem() noexcept {}

  // File Operations
  virtual FilePath    getAbsPath(const QString& path = "") const noexcept   = 0;
  virtual QStringList getDirs(const QString& path = "") const noexcept      = 0;
  virtual QStringList getFiles(const QString& path = "") const noexcept     = 0;
  virtual bool        fileExists(const QString& path) const noexcept        = 0;
  virtual QByteArray  read(const QString& path) const                       = 0;
  virtual void        write(const QString& path, const QByteArray& content) = 0;
  virtual void        removeFile(const QString& path)                       = 0;
  virtual void        removeDirRecursively(const QString& path = "")        = 0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_FILESYSTEM_H
