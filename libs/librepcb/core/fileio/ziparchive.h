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

#ifndef LIBREPCB_CORE_ZIPARCHIVE_H
#define LIBREPCB_CORE_ZIPARCHIVE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../utils/rusthandle.h"

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

namespace rs {
struct ZipArchive;
}

/*******************************************************************************
 *  Class ZipArchive
 ******************************************************************************/

/**
 * @brief Zip file reader
 *
 * @note This is just a wrapper around its Rust implementation.
 */
class ZipArchive final {
public:
  ZipArchive() = delete;
  ZipArchive(const ZipArchive& other) = delete;
  ZipArchive& operator=(const ZipArchive& rhs) = delete;

  /**
   * @brief Open in-memory Zip from `QByteArray`
   *
   * @param data  Raw Zip content.
   *
   * @throws Exception on any error
   */
  ZipArchive(const QByteArray& data);

  /**
   * @brief Open Zip file
   *
   * @param fp    Path to Zip file.
   *
   * @throws Exception on any error
   */
  ZipArchive(const FilePath& fp);

  /**
   * @brief Get number of ZIp entries (files & directories)
   *
   * @return Number of entries
   */
  std::size_t getEntriesCount() const noexcept;

  /**
   * @brief Get the file name of an entry
   *
   * @param index Entry index
   *
   * @return Entry file name (directory if it ends with slash or backslash)
   *
   * @throws  On invalid index or if the entry file name is somehow invalid
   *          or dangerous (zeros in its name, path outside Zip, ...).
   */
  QString getFileName(std::size_t index);

  /**
   * @brief Read the content of a file
   *
   * @param index Entry index
   *
   * @return File content
   *
   * @throws On invalid index or I/O errors.
   */
  QByteArray readFile(std::size_t index);

  /**
   * @brief Convenience method to find a file by name and read its content
   *
   * @param fileName  Relative file name to read.
   *
   * @return The file content if file was found, `std::nullopt` otherwise.
   *
   * @throws On I/O errors.
   */
  std::optional<QByteArray> tryReadFile(const QString& fileName);

  /**
   * @brief Extract whole Zip to a directory (overwriting)
   *
   * @param dir   Target directory
   *
   * @throws  On invalid/unsafe Zip content or I/O errors. In that case, the
   *          extraction might have been only partially (not atomically).
   */
  void extractTo(const FilePath& dir);

private:
  RustHandle<rs::ZipArchive> mHandle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
