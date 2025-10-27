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

#ifndef LIBREPCB_CORE_CORPORATECHECKMESSAGES_H
#define LIBREPCB_CORE_CORPORATECHECKMESSAGES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../rulecheck/rulecheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class MsgInvalidImageFile
 ******************************************************************************/

/**
 * @brief The MsgInvalidImageFile class
 */
class MsgInvalidImageFile final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgInvalidImageFile)

public:
  enum class Error {
    FileMissing,
    FileReadError,
    UnsupportedFormat,
    ImageLoadError,
  };

  // Constructors / Destructor
  MsgInvalidImageFile() = delete;
  explicit MsgInvalidImageFile(const QString& fileName, Error error,
                               const QString& details) noexcept;
  MsgInvalidImageFile(const MsgInvalidImageFile& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgInvalidImageFile() noexcept {}

private:
  static QString buildMessagePattern(Error error) noexcept;
  static QString buildDescription(const QString& details) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
