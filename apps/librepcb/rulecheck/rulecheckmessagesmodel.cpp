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
#include "rulecheckmessagesmodel.h"

#include "../apptoolbox.h"
#include "../uitypes.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RuleCheckMessagesModel::RuleCheckMessagesModel(QObject* parent) noexcept
  : QObject(parent) {
}

RuleCheckMessagesModel::~RuleCheckMessagesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RuleCheckMessagesModel::setMessages(
    const RuleCheckMessageList& messages,
    const QSet<SExpression>& approvals) noexcept {
  mMessages = messages;
  mApprovals = approvals;
  reset();
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t RuleCheckMessagesModel::row_count() const {
  return mMessages.size();
}

std::optional<ui::RuleCheckMessageData> RuleCheckMessagesModel::row_data(
    std::size_t i) const {
  if (auto msg = mMessages.value(i)) {
    return ui::RuleCheckMessageData{
        l2s(msg->getSeverity()),  // Severity
        q2s(msg->getMessage()),  // Message
        q2s(msg->getDescription()),  // Description
        mApprovals.contains(msg->getApproval()),  // Approved
        true,  // TODO: Supports autofix
        false,  // Autofix requested
    };
  } else {
    return std::nullopt;
  }
}

void RuleCheckMessagesModel::set_row_data(
    std::size_t i, const ui::RuleCheckMessageData& data) noexcept {
  if (auto msg = mMessages.value(i)) {
    if (data.approved && (!mApprovals.contains(msg->getApproval()))) {
      mApprovals.insert(msg->getApproval());
      emit approvalChanged(msg->getApproval(), true);
      row_changed(i);
    } else if ((!data.approved) && mApprovals.contains(msg->getApproval())) {
      mApprovals.remove(msg->getApproval());
      emit approvalChanged(msg->getApproval(), false);
      row_changed(i);
    } else if (data.autofix_requested) {
      emit autofixRequested(msg);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
