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
#include "imagehelpers.h"

#include "../dialogs/filedialog.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/geometry/image.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Helper Methods
 ******************************************************************************/

static bool convertToSupportedFormat(QByteArray& data,
                                     QString& format) noexcept {
  if (Image::getSupportedExtensions().contains(format)) {
    return true;
  }

  // We don't allow "jpeg", only "jpg".
  if (format == "jpeg") {
    format = "jpg";
    return true;
  }

  // For any non-supported file format, we have to convert the image to
  // a supported format. We use PNG for images with an alpha channel (to keep
  // transparent areas), and JPEG for any other images (to avoid very large
  // image files for things like photos).
  qInfo() << "Image format" << format
          << "is not natively supported, will be converted...";
  QImage img;
  if ((!img.loadFromData(data, qPrintable(format))) || img.isNull() ||
      (img.width() < 1) || (img.height() < 1)) {
    return false;
  }
  data.clear();  // Will be overwritten below.
  QBuffer buffer(&data);
  buffer.open(QIODevice::WriteOnly);
  const char* formatStr = img.hasAlphaChannel() ? "PNG" : "JPG";
  img.save(&buffer, formatStr);
  format = QString(formatStr).toLower();

  Q_ASSERT(Image::getSupportedExtensions().contains(format));
  return true;
}

static std::optional<FilePath> tryGetImageFilePathFromClipboard(
    const QMimeData* d) noexcept {
  const QString uriStr = QString(d->data("text/uri-list")).replace("\r", "");
  const QStringList uriList = uriStr.split("\n", Qt::SkipEmptyParts);
  if (uriList.count() == 1) {
    const QUrl url(uriList.first(), QUrl::StrictMode);
    if (url.isValid() && url.isLocalFile()) {
      const FilePath fp(url.toLocalFile());
      const QString format = fp.getSuffix().toLower();
      if (fp.isValid() &&
          QImageReader::supportedImageFormats().contains(format) &&
          fp.isExistingFile()) {
        return fp;
      }
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

std::optional<FileProofName> ImageHelpers::findExistingFile(
    const TransactionalDirectory& dir, const QByteArray& data) {
  for (const QString& name : dir.getFiles()) {
    const QString ext = name.split(".").last();
    if (!Image::getSupportedExtensions().contains(ext)) continue;
    if (!FileProofNameConstraint()(name)) continue;  // Invalid filename.
    if (dir.read(name) == data) {
      return FileProofName(name);
    }
  }
  return std::nullopt;
}

FileProofName ImageHelpers::getUnusedFileName(const TransactionalDirectory& dir,
                                              QString nameUserInput,
                                              const QString& extension) {
  if (!Image::getSupportedExtensions().contains(extension)) {
    throw LogicError(__FILE__, __LINE__);
  }

  QString suffix = "." % extension;
  nameUserInput = cleanFileProofName(nameUserInput.trimmed());
  if (nameUserInput.isEmpty()) {
    nameUserInput = "image";  // Fallback / default for clipboard images.
  }

  int i = 2;
  QString fileName;
  do {
    nameUserInput.truncate(FileProofNameConstraint::MAX_LEN - suffix.length());
    fileName = nameUserInput % suffix;
    suffix = "-" % QString::number(i) % "." % extension;
    ++i;
  } while (dir.fileExists(fileName));

  return FileProofName(fileName);
}

bool ImageHelpers::execImageChooserDialog(QByteArray& data, QString& format,
                                          QString& basename,
                                          const QString& settingsKey) {
  QSettings cs;
  const QString selectedFile =
      cs.value(settingsKey, QDir::homePath()).toString();

  QList<QByteArray> filterTypes = QImageReader::supportedImageFormats();
  for (QByteArray& s : filterTypes) {
    s.prepend("*.");
  }
  const QString filter = tr("Image Files") % " (" % filterTypes.join(" ") % ")";
  const FilePath fp(FileDialog::getOpenFileName(
      qApp->activeWindow(), tr("Choose Image File"), selectedFile, filter));
  if (!fp.isValid()) {
    return false;
  }

  cs.setValue(settingsKey, fp.toStr());

  data = FileUtils::readFile(fp);  // can throw
  format = fp.getSuffix().toLower();
  basename = fp.getCompleteBasename();

  // Make sure the format is supported (will be converted if needed).
  if (!convertToSupportedFormat(data, format)) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Failed to convert image '%1' to a supported "
                               "format. Please try a different image format.")
                           .arg(fp.toNative()));
  }

  return true;
}

std::optional<FileProofName> ImageHelpers::findExistingOrAskForNewImageFileName(
    const TransactionalDirectory& dir, Target target, const QByteArray& data,
    const QString& format, const QString& basename, bool& exists) {
  if (auto f = findExistingFile(dir, data)) {
    exists = true;
    return f;
  }

  exists = false;

  // Determine file name of new file to be created. Give the user the
  // chance to modify the filename as sometimes they are nonsense
  // (e.g. "Screenshot from yyyy-mm-dd hh:mm.png").
  const FileProofName name = getUnusedFileName(dir, basename, format);
  const int maxLen = FileProofNameConstraint::MAX_LEN - format.length() - 1;
  QString msg;
  if (target == Target::Symbol) {
    msg = tr("The image will be copied into the symbol as a %1 file.")
              .arg("*." % format);
  } else {
    msg = tr("The image will be copied into the project as a %1 file.")
              .arg("*." % format);
  }
  msg += "\n\n";
  msg += tr("Basename of the new file (max. %n characters):", nullptr, maxLen);
  QInputDialog dlg(qApp->activeWindow());
  dlg.setInputMode(QInputDialog::TextInput);
  dlg.setWindowTitle(tr("Image Name"));
  dlg.setLabelText(msg);
  dlg.setTextValue(name->chopped(format.length() + 1));
  if (auto lineEdit = dlg.findChild<QLineEdit*>()) {
    lineEdit->setMaxLength(maxLen);
    lineEdit->setValidator(
        new QRegularExpressionValidator(FileProofNameConstraint::regex()));
  }
  if (dlg.exec() != QDialog::Accepted) return std::nullopt;

  // Make sure the filename is really valid and nonexistent.
  return getUnusedFileName(dir, dlg.textValue(), format);
}

bool ImageHelpers::isImageInClipboard() noexcept {
  const QMimeData* d = qApp->clipboard()->mimeData();

  // Important: If there is any LibrePCB data in the clipboard, do *NOT*
  // consider it as an image! LibrePCB does sometimes put image data into
  // the clipboard when copying things (e.g. in the symbol editor). But
  // when pasting, we want that *data* to be pasted, not the image.
  bool hasImageData = false;
  for (const QString& format : d->formats()) {
    if (format.startsWith("application/x-librepcb-clipboard")) {
      return false;
    } else if (format.startsWith("image/")) {
      hasImageData = true;
    }
  }
  if (hasImageData) {
    return true;
  }

  // Check if we have an image file path in the clipboard.
  if (tryGetImageFilePathFromClipboard(d)) {
    return true;
  }

  return false;
}

bool ImageHelpers::getImageFromClipboard(QByteArray& data, QString& format,
                                         QString& basename) noexcept {
  const QMimeData* d = qApp->clipboard()->mimeData();

  // Important: If there is any LibrePCB data in the clipboard, do *NOT*
  // consider it as an image! LibrePCB does sometimes put image data into
  // the clipboard when copying things (e.g. in the symbol editor). But
  // when pasting, we want that *data* to be pasted, not the image.
  for (const QString& format : d->formats()) {
    if (format.startsWith("application/x-librepcb-clipboard")) {
      return false;
    }
  }

  // If there is an SVG, we should priorize it over the pixmap formats.
  data = d->data("image/svg+xml");
  if (!data.isEmpty()) {
    format = "svg";
    return true;
  }

  // Heuristic to choose between PNG or JPEG.
  const QImage img = qApp->clipboard()->image();
  auto choosePngOrJpg = [&data, &format, &img](const QByteArray& png,
                                               const QByteArray& jpg) noexcept {
    auto hasTransparency = [&img]() {
      if (img.isNull() || img.hasAlphaChannel()) {
        return false;
      }
      for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
          if (qAlpha(img.pixel(x, y)) < 255) {
            return true;
          }
        }
      }
      return false;
    };
    const QString debugSuffix = QString("(png=%1kB, jpg=%2kB).")
                                    .arg(png.size() / 1024)
                                    .arg(jpg.size() / 1024);
    if (png.size() < (5 * jpg.size())) {
      qDebug().noquote() << "Using clipboard image as PNG" << debugSuffix;
      data = png;
      format = "png";
    } else if (hasTransparency()) {
      qDebug().noquote() << "Using clipboard image as PNG due to transparency"
                         << debugSuffix;
      data = png;
      format = "png";
    } else {
      qDebug().noquote() << "Using clipboard image as JPEG" << debugSuffix;
      data = jpg;
      format = "jpg";
    }
  };

  // If both PNG and JPEG are provided (which seems to be the case often),
  // we choose the most reasonable depending on their size and alpha channel.
  QByteArray png = d->data("image/png");
  QByteArray jpg = d->data("image/jpeg");
  if ((!png.isEmpty()) && (!jpg.isEmpty())) {
    choosePngOrJpg(png, jpg);
    return true;
  }

  // If either PNG or JPEG is available, use it without conversion.
  if (!png.isEmpty()) {
    data = png;
    format = "png";
    return true;
  }
  if (!jpg.isEmpty()) {
    data = jpg;
    format = "jpg";
    return true;
  }

  // Try other images and convert to either PNG or JPEG.
  auto imgToByteArray = [&img](const char* fmt) {
    QByteArray b;
    QBuffer buffer(&b);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, fmt);
    return b;
  };
  if (!img.isNull()) {
    png = imgToByteArray("PNG");
    jpg = imgToByteArray("JPEG");
    choosePngOrJpg(png, jpg);
    return true;
  }

  // Try local file path (only if a single file is in clipboard).
  if (auto fp = tryGetImageFilePathFromClipboard(d)) {
    QFile file(fp->toStr());
    if (file.open(QIODevice::ReadOnly)) {
      data = file.readAll();
      format = fp->getSuffix().toLower();
      basename = fp->getCompleteBasename();
      return convertToSupportedFormat(data, format);
    }
  }

  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
