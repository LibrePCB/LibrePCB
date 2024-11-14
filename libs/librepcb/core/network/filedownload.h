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

#ifndef LIBREPCB_CORE_FILEDOWNLOAD_H
#define LIBREPCB_CORE_FILEDOWNLOAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"
#include "networkrequestbase.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class FileDownload
 ******************************************************************************/

/**
 * @brief This class is used to download a file asynchronously in a separate
 * thread
 *
 * @see librepcb::NetworkRequestBase, librepcb::DownloadManager
 */
class FileDownload final : public NetworkRequestBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  FileDownload() = delete;
  FileDownload(const FileDownload& other) = delete;

  /**
   * @brief Constructor
   *
   * @param url           The URL to the file to download
   * @param dest          The path to the destination file (must not exist!)
   */
  FileDownload(const QUrl& url, const FilePath& dest) noexcept;

  ~FileDownload() noexcept;

  // Setters

  /**
   * @brief Set the expected checksum of the file to download
   *
   * If set, the checksum of the downloaded file will be compared with this
   * checksum. If they differ, the file gets removed and an error will be
   * reported.
   *
   * @param algorithm     The checksum algorithm to be used
   * @param checksum      The expected checksum of the file to download
   */
  void setExpectedChecksum(QCryptographicHash::Algorithm algorithm,
                           const QByteArray& checksum) noexcept;

  /**
   * @brief Set extraction directory of the ZIP file to download
   *
   * If set (and valid), the downloaded file (must be a ZIP!) will be extracted
   * into this directory after downloading it.
   *
   * @note The downloaded ZIP file will be removed after extracting it.
   *
   * @param dir           Destination directory (may or may not exist)
   */
  void setZipExtractionDirectory(const FilePath& dir) noexcept;

  // Operator Overloadings
  FileDownload& operator=(const FileDownload& rhs) = delete;

signals:

  /**
   * @brief File successfully downloaded signal (emitted right before
   * #finished())
   *
   * @note The parameter type is specified with the full namespace, reason see
   * here:
   *       http://stackoverflow.com/questions/21119397/emitting-signals-with-custom-types-does-not-work
   */
  void fileDownloaded(librepcb::FilePath filepath);

  /**
   * @brief ZIP file successfully extracted signal (emitted right before
   * #finished())
   *
   * @note The parameter type is specified with the full namespace, reason see
   * here:
   *       http://stackoverflow.com/questions/21119397/emitting-signals-with-custom-types-does-not-work
   */
  void zipFileExtracted(librepcb::FilePath directory);

private:  // Methods
  void prepareRequest() override;
  void finalizeRequest() override;
  void emitSuccessfullyFinishedSignals(QString contentType) noexcept override;
  void fetchNewData(QIODevice& device) noexcept override;

private:  // Data
  FilePath mDestination;
  QScopedPointer<QSaveFile> mFile;
  QCryptographicHash::Algorithm mHashAlgorithm;
  QByteArray mExpectedChecksum;
  FilePath mExtractZipToDir;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
