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

#include "../utils/slinthelpers.h"
#include "../utils/uihelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RuleCheckMessagesModel::RuleCheckMessagesModel(QObject* parent) noexcept
  : QObject(parent), mUnapprovedCount(0), mErrorCount(0) {
}

RuleCheckMessagesModel::~RuleCheckMessagesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RuleCheckMessagesModel::clear() noexcept {
  mMessages.clear();
  mApprovals.clear();
  notify_reset();
  updateCounters();
}

void RuleCheckMessagesModel::setAutofixHandler(
    AutofixHandler handler) noexcept {
  mAutofixHandler = handler;
  notify_reset();
}

void RuleCheckMessagesModel::setMessages(
    const RuleCheckMessageList& messages,
    const QSet<SExpression>& approvals) noexcept {
  mMessages = messages;
  mApprovals = approvals;
  notify_reset();
  updateCounters();
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
        mAutofixHandler && mAutofixHandler(msg, true),  // Supports autofix
        ui::RuleCheckMessageAction::None,  // Action
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
      notify_row_changed(i);
      updateCounters();
    } else if ((!data.approved) && mApprovals.contains(msg->getApproval())) {
      mApprovals.remove(msg->getApproval());
      emit approvalChanged(msg->getApproval(), false);
      notify_row_changed(i);
      updateCounters();
    } else if (data.action == ui::RuleCheckMessageAction::Highlight) {
      emit highlightRequested(msg, false);
    } else if (data.action == ui::RuleCheckMessageAction::HighlightAndZoomTo) {
      emit highlightRequested(msg, true);
    } else if (data.action == ui::RuleCheckMessageAction::Autofix) {
      QMetaObject::invokeMethod(
          this,
          [this, msg]() {
            if (mAutofixHandler) {
              mAutofixHandler(msg, false);
            }
          },
          Qt::QueuedConnection);
    } else if (data.action != ui::RuleCheckMessageAction::None) {
      qWarning() << "Unhandled action in RuleCheckMessagesModel:"
                 << static_cast<int>(data.action);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RuleCheckMessagesModel::updateCounters() noexcept {
  int unapproved = 0;
  int errors = 0;
  for (auto msg : mMessages) {
    if (!mApprovals.contains(msg->getApproval())) {
      ++unapproved;
    }
    if (msg->getSeverity() == RuleCheckMessage::Severity::Error) {
      ++errors;
    }
  }
  if (unapproved != mUnapprovedCount) {
    mUnapprovedCount = unapproved;
    emit unapprovedCountChanged(mUnapprovedCount);
  }
  if (errors != mErrorCount) {
    mErrorCount = errors;
    emit errorCountChanged(mErrorCount);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
