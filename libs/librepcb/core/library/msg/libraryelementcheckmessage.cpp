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
#include "libraryelementcheckmessage.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElementCheckMessage::LibraryElementCheckMessage(
    const LibraryElementCheckMessage& other) noexcept
  : mSeverity(other.mSeverity),
    mSeverityPixmap(other.mSeverityPixmap),
    mMessage(other.mMessage),
    mDescription(other.mDescription),
    mApproval(other.mApproval) {
}

LibraryElementCheckMessage::LibraryElementCheckMessage(
    Severity severity, const QString& msg, const QString& description,
    const QString& approvalName) noexcept
  : mSeverity(severity),
    mSeverityPixmap(getSeverityPixmap(severity)),
    mMessage(msg),
    mDescription(description),
    mApproval(SExpression::createList("approved")) {
  mApproval.appendChild(SExpression::createToken(approvalName));  // snake_case
}

LibraryElementCheckMessage::~LibraryElementCheckMessage() noexcept {
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QPixmap LibraryElementCheckMessage::getSeverityPixmap(
    Severity severity) noexcept {
  static QMap<Severity, QPixmap> pixmap = {
      {Severity::Hint, QPixmap(":/img/status/info.png")},
      {Severity::Warning, QPixmap(":/img/status/dialog_warning.png")},
      {Severity::Error, QPixmap(":/img/status/dialog_error.png")},
  };
  return pixmap.value(severity);
}

/*******************************************************************************
 *  Operator Overloads
 ******************************************************************************/

bool LibraryElementCheckMessage::operator==(
    const LibraryElementCheckMessage& rhs) const noexcept {
  if (mSeverity != rhs.mSeverity) return false;
  if (mMessage != rhs.mMessage) return false;
  if (mDescription != rhs.mDescription) return false;
  return true;
}

bool LibraryElementCheckMessage::operator!=(
    const LibraryElementCheckMessage& rhs) const noexcept {
  return !(*this == rhs);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
