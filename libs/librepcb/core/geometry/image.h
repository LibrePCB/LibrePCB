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

#ifndef LIBREPCB_CORE_IMAGE_H
#define LIBREPCB_CORE_IMAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "../types/angle.h"
#include "../types/fileproofname.h"
#include "../types/length.h"
#include "../types/point.h"

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Image
 ******************************************************************************/

/**
 * @brief The Image class
 */
class Image final {
  Q_DECLARE_TR_FUNCTIONS(Image)

public:
  // Signals
  enum class Event {
    UuidChanged,
    FileNameChanged,
    PositionChanged,
    RotationChanged,
    WidthChanged,
    HeightChanged,
    BorderWidthChanged,
  };
  Signal<Image, Event> onEdited;
  typedef Slot<Image, Event> OnEditedSlot;

  // Constructors / Destructor
  Image() = delete;
  Image(const Image& other) noexcept;
  Image(const Uuid& uuid, const Image& other) noexcept;
  Image(const Uuid& uuid, const FileProofName& fileName, const Point& pos,
        const Angle& rotation, const PositiveLength& width,
        const PositiveLength& height,
        const std::optional<UnsignedLength>& borderWidth) noexcept;
  explicit Image(const SExpression& node);
  ~Image() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const FileProofName& getFileName() const noexcept { return mFileName; }
  QString getFileBasename() const noexcept;
  QString getFileExtension() const noexcept;
  const Point& getPosition() const noexcept { return mPosition; }
  Point getCenter() const noexcept;
  const Angle& getRotation() const noexcept { return mRotation; }
  const PositiveLength& getWidth() const noexcept { return mWidth; }
  const PositiveLength& getHeight() const noexcept { return mHeight; }
  const std::optional<UnsignedLength>& getBorderWidth() const noexcept {
    return mBorderWidth;
  }

  // Setters
  bool setFileName(const FileProofName& name) noexcept;
  bool setPosition(const Point& pos) noexcept;
  bool setRotation(const Angle& rotation) noexcept;
  bool setWidth(const PositiveLength& width) noexcept;
  bool setHeight(const PositiveLength& height) noexcept;
  bool setBorderWidth(const std::optional<UnsignedLength>& width) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Image& rhs) const noexcept;
  bool operator!=(const Image& rhs) const noexcept { return !(*this == rhs); }
  Image& operator=(const Image& rhs) noexcept;

  // Static Methods.

  /**
   * @brief Get all supported file extensions
   *
   * @note  This is only a small subset of Qt's supported image formats. We
   *        don't want to support unusual, exotic or non-portable image formats
   *        to make sure LibrePCB can open them on any platform without needing
   *        possibly heavy dependencies.
   *
   * @note  All returned file extensions are lowercase and we expect images
   *        to be created only with lowercase file extensions too.
   *
   * @return File extensions (e.g. "png", "jpg", "svg").
   */
  static const QStringList& getSupportedExtensions() noexcept;

  /**
   * @brief Try loading an image file
   *
   * @note  This also verifies that the format is officially supported. Formats
   *        not contained in #getSupportedExtensions() will return
   *        `std::nullopt` even if Qt would be able to load it.
   *
   * @param data      The file content.
   * @param format    The file format (#getFileExtension()). Note that we are
   *                  case-sensitive, i.e. don't allow uppercase file extensions
   *                  (there's no good reason to have uppercase file
   *                  extensions).
   * @param errorMsg  If not `nullptr`, the (translated) error message will be
   *                  written into this string (only on error).
   * @return The `QImage` on success, `std::nullopt` on failure.
   */
  static std::optional<QImage> tryLoad(const QByteArray& data,
                                       const QString& format,
                                       QString* errorMsg = nullptr) noexcept;

private:  // Data
  Uuid mUuid;
  FileProofName mFileName;
  Point mPosition;
  Angle mRotation;
  PositiveLength mWidth;
  PositiveLength mHeight;
  std::optional<UnsignedLength> mBorderWidth;
};

/*******************************************************************************
 *  Class ImageList
 ******************************************************************************/

struct ImageListNameProvider {
  static constexpr const char* tagname = "image";
};
using ImageList =
    SerializableObjectList<Image, ImageListNameProvider, Image::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
