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

#ifndef LIBREPCB_CORE_OUTPUTDIRECTORYWRITER_H
#define LIBREPCB_CORE_OUTPUTDIRECTORYWRITER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Uuid;

/*******************************************************************************
 *  Class OutputDirectoryWriter
 ******************************************************************************/

/**
 * @brief The OutputDirectoryWriter class
 */
class OutputDirectoryWriter final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  OutputDirectoryWriter() = delete;
  OutputDirectoryWriter(const OutputDirectoryWriter& other) = delete;
  explicit OutputDirectoryWriter(const FilePath& dirPath) noexcept;
  ~OutputDirectoryWriter() noexcept;

  // Getters
  const FilePath& getDirectoryPath() const noexcept { return mDirPath; }
  const QMultiHash<Uuid, FilePath>& getWrittenFiles() const noexcept {
    return mWrittenFiles;
  }

  // General Methods
  bool loadIndex();
  void storeIndex();
  FilePath beginWritingFile(const Uuid& job, const QString& relPath);
  void removeObsoleteFiles(const Uuid& job);
  QList<FilePath> findUnknownFiles(const QSet<Uuid>& knownJobs) const;
  void removeUnknownFiles(const QList<FilePath>& files);

  // Operator Overloadings
  OutputDirectoryWriter& operator=(const OutputDirectoryWriter& rhs) = delete;

signals:
  void aboutToWriteFile(const FilePath& fp);
  void aboutToRemoveFile(const FilePath& fp);

private:  // Data
  const FilePath mDirPath;
  const FilePath mIndexFilePath;
  QMap<FilePath, Uuid> mIndex;
  bool mIndexLoaded;
  bool mIndexModified;
  QMultiHash<Uuid, FilePath> mWrittenFiles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
