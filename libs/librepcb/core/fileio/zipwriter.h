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

#ifndef LIBREPCB_CORE_ZIPWRITER_H
#define LIBREPCB_CORE_ZIPWRITER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../utils/rusthandle.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

namespace rs {
struct ZipWriter;
}

/*******************************************************************************
 *  Class ZipWriter
 ******************************************************************************/

/**
 * @brief Zip file writer
 *
 * @note This is just a wrapper around its Rust implementation.
 */
class ZipWriter final {
public:
  ZipWriter(const ZipWriter& other) = delete;
  ZipWriter& operator=(const ZipWriter& rhs) = delete;

  /**
   * @brief Create in-memory writer
   *
   * @throws Exception on any error
   */
  ZipWriter();

  /**
   * @brief Create file writer
   *
   * @param fp    Path to Zip file.
   *
   * @throws Exception on any error
   */
  ZipWriter(const FilePath& fp);

  /**
   * @brief Destructor
   */
  ~ZipWriter() noexcept;

  /**
   * @brief Write a file
   *
   * @param path  Path within archive
   * @param data  File content
   * @param mode  Unix permissions (e.g. 0644).
   */
  void writeFile(const QString& path, const QByteArray& data, uint32_t mode);

  /**
   * @brief Finish writing the file
   *
   * This must always be called after all files have been written.
   *
   * @throws On I/O errors.
   */
  void finish();

  /**
   * @brief Get the in-memory ZIP data
   *
   * @note #finish() must have been called first.
   *
   * @return The ZIP content
   *
   * @throws  If the written data is empty or incomplete, or this is not an
   *          in-memory writer.
   */
  const QByteArray& getData() const;

private:
  QByteArray mBuffer;
  RustHandle<rs::ZipWriter> mHandle;
  bool mFinished;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
