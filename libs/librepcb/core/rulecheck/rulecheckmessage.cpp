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
#include "rulecheckmessage.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RuleCheckMessage::RuleCheckMessage(const RuleCheckMessage& other) noexcept
  : mSeverity(other.mSeverity),
    mSeverityIcon(other.mSeverityIcon),
    mMessage(other.mMessage),
    mDescription(other.mDescription),
    mApproval(other.mApproval) {
}

RuleCheckMessage::RuleCheckMessage(Severity severity, const QString& msg,
                                   const QString& description,
                                   const QString& approvalName) noexcept
  : mSeverity(severity),
    mSeverityIcon(getSeverityIcon(severity)),
    mMessage(msg),
    mDescription(description),
    mApproval(SExpression::createList("approved")) {
  mApproval.appendChild(SExpression::createToken(approvalName));  // snake_case
}

RuleCheckMessage::~RuleCheckMessage() noexcept {
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QIcon RuleCheckMessage::getSeverityIcon(Severity severity) noexcept {
  static QMap<Severity, QIcon> icon = {
      {Severity::Hint, QIcon(":/img/status/info.png")},
      {Severity::Warning, QIcon(":/img/status/dialog_warning.png")},
      {Severity::Error, QIcon(":/img/status/dialog_error.png")},
  };
  return icon.value(severity);
}

/*******************************************************************************
 *  Operator Overloads
 ******************************************************************************/

bool RuleCheckMessage::operator==(const RuleCheckMessage& rhs) const noexcept {
  if (mSeverity != rhs.mSeverity) return false;
  if (mMessage != rhs.mMessage) return false;
  if (mDescription != rhs.mDescription) return false;
  return true;
}

bool RuleCheckMessage::operator!=(const RuleCheckMessage& rhs) const noexcept {
  return !(*this == rhs);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
