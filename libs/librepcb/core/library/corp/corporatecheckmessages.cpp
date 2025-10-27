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
#include "corporatecheckmessages.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  MsgInvalidImageFile
 ******************************************************************************/

MsgInvalidImageFile::MsgInvalidImageFile(const QString& fileName, Error error,
                                         const QString& details) noexcept
  : RuleCheckMessage(Severity::Error, buildMessagePattern(error).arg(fileName),
                     buildDescription(details), "invalid_image_file") {
  mApproval->appendChild("file", fileName);
}

QString MsgInvalidImageFile::buildMessagePattern(Error error) noexcept {
  QHash<Error, QString> translations = {
      {Error::FileMissing, tr("Missing image file: '%1'")},
      {Error::FileReadError, QString("Failed to read image file: '%1'")},
      {Error::UnsupportedFormat, tr("Unsupported image format: '%1'")},
      {Error::ImageLoadError, tr("Invalid image file: '%1'")},
  };
  return translations.value(error, "Unknown image error: %1");
}

QString MsgInvalidImageFile::buildDescription(const QString& details) noexcept {
  QString s =
      tr("The referenced file of an image does either not exist in the symbol "
         "or is not a valid image file. Try removing and re-adding the image "
         "from the symbol.");
  if (!details.isEmpty()) {
    s += "\n\n" % tr("Details:") % " " % details;
  }
  return s;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
