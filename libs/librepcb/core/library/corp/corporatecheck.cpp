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
#include "corporatecheck.h"

#include "corporate.h"
#include "corporatecheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CorporateCheck::CorporateCheck(const Corporate& Corporate) noexcept
  : LibraryBaseElementCheck(Corporate), mCorporate(Corporate) {
}

CorporateCheck::~CorporateCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList CorporateCheck::runChecks() const {
  RuleCheckMessageList msgs = LibraryBaseElementCheck::runChecks();
  checkInvalidImageFiles(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void CorporateCheck::checkInvalidImageFiles(MsgList& msgs) const {
  /*using Error = MsgInvalidImageFile::Error;
  typedef std::optional<std::pair<Error, QString>> Result;

  auto getError = [this](const Image& image) {
    try {
      if (!mCorporate.getDirectory().fileExists(*image.getFileName())) {
        return std::make_optional(
            std::make_pair(Error::FileMissing, QString()));
      }
      const QByteArray content =
          mCorporate.getDirectory().read(*image.getFileName());  // can throw
      QString error = "Unknown error.";
      if (Image::tryLoad(content, image.getFileExtension(), &error)) {
        return Result();  // Success.
      } else if (!Image::getSupportedExtensions().contains(
                     image.getFileExtension())) {
        return std::make_optional(
            std::make_pair(Error::UnsupportedFormat, error));
      } else {
        return std::make_optional(std::make_pair(Error::ImageLoadError, error));
      }
    } catch (const Exception& e) {
      return std::make_optional(
          std::make_pair(Error::FileReadError, e.getMsg()));
    }
  };

  // Emit the warning only once per filename.
  QMap<QString, Result> errors;
  for (const Image& image : mCorporate.getImages()) {
    if (!errors.contains(*image.getFileName())) {
      errors.insert(*image.getFileName(), getError(image));
    }
  }
  for (auto it = errors.begin(); it != errors.end(); it++) {
    if (it.value()) {
      msgs.append(std::make_shared<MsgInvalidImageFile>(
          it.key(), it.value()->first, it.value()->second));
    }
  }*/
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
