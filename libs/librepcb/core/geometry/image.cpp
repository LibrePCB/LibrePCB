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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "image.h"

#include <QSvgRenderer>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(
    const std::optional<UnsignedLength>& obj) {
  return obj ? serialize(*obj) : SExpression::createToken("none");
}

template <>
std::optional<UnsignedLength> deserialize(const SExpression& node) {
  if (node.getValue() == "none") {
    return std::nullopt;
  } else {
    return UnsignedLength(deserialize<Length>(node));  // can throw
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Image::Image(const Image& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mFileName(other.mFileName),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mWidth(other.mWidth),
    mHeight(other.mHeight),
    mBorderWidth(other.mBorderWidth) {
}

Image::Image(const Uuid& uuid, const Image& other) noexcept : Image(other) {
  mUuid = uuid;
}

Image::Image(const Uuid& uuid, const FileProofName& fileName, const Point& pos,
             const Angle& rotation, const PositiveLength& width,
             const PositiveLength& height,
             const std::optional<UnsignedLength>& borderWidth) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mFileName(fileName),
    mPosition(pos),
    mRotation(rotation),
    mWidth(width),
    mHeight(height),
    mBorderWidth(borderWidth) {
}

Image::Image(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mFileName(deserialize<FileProofName>(node.getChild("file/@0"))),
    mPosition(node.getChild("position")),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"))),
    mWidth(deserialize<PositiveLength>(node.getChild("width/@0"))),
    mHeight(deserialize<PositiveLength>(node.getChild("height/@0"))),
    mBorderWidth(deserialize<std::optional<UnsignedLength>>(
        node.getChild("border/@0"))) {
}

Image::~Image() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString Image::getFileBasename() const noexcept {
  QStringList parts = mFileName->split(".", Qt::KeepEmptyParts);
  if (parts.count() > 1) {
    parts.removeLast();
  }
  return parts.join(".");
}

QString Image::getFileExtension() const noexcept {
  return mFileName->split(".", Qt::KeepEmptyParts).last();
}

Point Image::getCenter() const noexcept {
  return mPosition + Point(mWidth / 2, mHeight / 2).rotated(mRotation);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Image::setFileName(const FileProofName& name) noexcept {
  if (name == mFileName) {
    return false;
  }

  mFileName = name;
  onEdited.notify(Event::FileNameChanged);
  return true;
}

bool Image::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool Image::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) {
    return false;
  }

  mRotation = rotation;
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool Image::setWidth(const PositiveLength& width) noexcept {
  if (width == mWidth) {
    return false;
  }

  mWidth = width;
  onEdited.notify(Event::WidthChanged);
  return true;
}

bool Image::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  onEdited.notify(Event::HeightChanged);
  return true;
}

bool Image::setBorderWidth(
    const std::optional<UnsignedLength>& width) noexcept {
  if (width == mBorderWidth) {
    return false;
  }

  mBorderWidth = width;
  onEdited.notify(Event::BorderWidthChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Image::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("file", *mFileName);
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  root.appendChild("width", mWidth);
  root.appendChild("height", mHeight);
  root.ensureLineBreak();
  root.appendChild("border", mBorderWidth);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Image::operator==(const Image& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mFileName != rhs.mFileName) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mWidth != rhs.mWidth) return false;
  if (mHeight != rhs.mHeight) return false;
  if (mBorderWidth != rhs.mBorderWidth) return false;
  return true;
}

Image& Image::operator=(const Image& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setFileName(rhs.mFileName);
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setWidth(rhs.mWidth);
  setHeight(rhs.mHeight);
  setBorderWidth(rhs.mBorderWidth);
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

const QStringList& Image::getSupportedExtensions() noexcept {
  // These are the basic image formats which should be supported on any
  // platform and without Qt's Image Formats plugin. Though there would be more
  // common image formats (like BMP), we do some guidance here which formats
  // we want to allow or not. For example BMP requires unnecessary disk space
  // (and thus file I/O time), so users shall use PNG instead.
  //
  // Note that the formats here does not restrict the image file formats which
  // can actually be used. LibrePCB can just convert other image formats to one
  // of those when adding images to LibrePCB libraries/projects. So for the
  // user it's still possible to add e.g. a BMP file.
  //
  // Also note that there's no benefit in supporting both "jpg" and "jpeg"
  // file suffixes, so we just always use "jpg" (and enforce it). Editors
  // shall rename "jpeg" to "jpg" if a "jpeg" file was selected by the user.
  // The same applies for the capitalization - we allow only lowercase
  // extensions.
  //
  // ATTENTION: This list is considered as part of the file format
  // specification! Any change will requiring bumping the file format version!
  static const QStringList extensions = {"jpg", "png", "svg"};
  return extensions;
}

std::optional<QImage> Image::tryLoad(const QByteArray& data,
                                     const QString& format,
                                     QString* errorMsg) noexcept {
  if (!getSupportedExtensions().contains(format)) {
    if (errorMsg) {
      *errorMsg =
          tr("Unsupported image file format '%1'. Supported formats are: %2")
              .arg(format)
              .arg(getSupportedExtensions().join(", "));
    }
    return std::nullopt;
  }
  if (data.isEmpty()) {
    if (errorMsg) {
      // No tr() because it should be a very rare error.
      *errorMsg = "Image file seems to be empty (0 bytes).";
    }
    return std::nullopt;
  }

  QImage img;
  if (format == "svg") {
    QSvgRenderer renderer(data);
    const QSize svgSize = renderer.defaultSize();
    if ((svgSize.width() < 1) || (svgSize.height() < 1)) {
      if (errorMsg) {
        // No tr() because it should be a very rare error.
        *errorMsg = "The SVG's image size appears to be zero.";
      }
      return std::nullopt;
    }
    // Make sure the image has a width or height of at least 800px to
    // avoid pixelated rendering for SVGs, depending on their scaling.
    // This is an ugly hack for now, in future we should scale the image
    // on demand, depending on zoom level, printer resolution etc.
    const qreal scaleFactor = std::max(
        800 / qreal(std::max(svgSize.width(), svgSize.height())), qreal(1));
    img = QImage(svgSize * scaleFactor, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter painter(&img);
    renderer.render(&painter);
  } else if (!img.loadFromData(data, qPrintable(format))) {
    if (errorMsg) {
      // No tr() because it should be a very rare error.
      *errorMsg = QString(
                      "Failed to load the image. Please check that the file is "
                      "valid and the provided file extension '%1' is correct.")
                      .arg(format);
    }
    return std::nullopt;
  }

  if ((img.width() > 0) && (img.height() > 0)) {
    return img;
  } else {
    if (errorMsg) {
      // No tr() because it should be a very rare error.
      *errorMsg = "The loaded image seems to be empty.";
    }
    return std::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
