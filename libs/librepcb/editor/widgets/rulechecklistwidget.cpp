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
#include "rulechecklistwidget.h"

#include <librepcb/core/utils/toolbox.h>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RuleCheckListWidget::RuleCheckListWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mListWidget(new QListWidget(this)),
    mReadOnly(false),
    mHandler(nullptr),
    mApprovals(),
    mUnapprovedMessageCount(std::nullopt) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mListWidget.data());
  mListWidget->setStyleSheet(QString("QListWidget::item:selected{"
                                     "  background-color: %1;"
                                     "}")
                                 .arg(mListWidget->palette()
                                          .color(QPalette::Highlight)
                                          .name(QColor::HexArgb)));
  connect(mListWidget.data(), &QListWidget::currentItemChanged, this,
          &RuleCheckListWidget::currentItemChanged);
  connect(mListWidget.data(), &QListWidget::itemDoubleClicked, this,
          &RuleCheckListWidget::itemDoubleClicked);
  updateList();  // Ensure consistent GUI enabled state.
}

RuleCheckListWidget::~RuleCheckListWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void RuleCheckListWidget::setReadOnly(bool readOnly) noexcept {
  if (readOnly != mReadOnly) {
    mReadOnly = readOnly;
    updateList();
  }
}

void RuleCheckListWidget::setHandler(IF_RuleCheckHandler* handler) noexcept {
  mHandler = handler;
}

void RuleCheckListWidget::setMessages(
    const std::optional<RuleCheckMessageList>& messages) noexcept {
  if (messages != mMessages) {
    mMessages = messages;
    updateList();
  }
}

void RuleCheckListWidget::setApprovals(
    const QSet<SExpression>& approvals) noexcept {
  if (approvals != mApprovals) {
    mApprovals = approvals;
    updateList();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RuleCheckListWidget::updateList() noexcept {
  // Sort by approval state, severity and message.
  mDisplayedMessages = mMessages ? (*mMessages) : RuleCheckMessageList();
  Toolbox::sortNumeric(
      mDisplayedMessages,
      [this](const QCollator& cmp,
             const std::shared_ptr<const RuleCheckMessage>& lhs,
             const std::shared_ptr<const RuleCheckMessage>& rhs) {
        if (lhs && rhs) {
          const bool lhsApproved = mApprovals.contains(lhs->getApproval());
          const bool rhsApproved = mApprovals.contains(rhs->getApproval());
          if (lhsApproved != rhsApproved) {
            return rhsApproved;
          } else if (lhs->getSeverity() != rhs->getSeverity()) {
            return lhs->getSeverity() > rhs->getSeverity();
          } else {
            return cmp(lhs->getMessage(), rhs->getMessage());
          }
        } else {
          return false;
        }
      },
      Qt::CaseInsensitive, false);

  // Update list widget.
  mListWidget->setUpdatesEnabled(false);  // Avoid flicker.
  const bool signalsBlocked = blockSignals(true);
  mListWidget->clear();
  int unapprovedMessageCount = 0;
  foreach (const auto& msg, mDisplayedMessages) {
    QListWidgetItem* item = new QListWidgetItem();
    mListWidget->addItem(item);
    const bool approved = mApprovals.contains(msg->getApproval());
    RuleCheckListItemWidget* widget =
        new RuleCheckListItemWidget(msg, *this, approved);
    mListWidget->setItemWidget(item, widget);
    if (!approved) {
      ++unapprovedMessageCount;
    }
  }
  if (mMessages && mMessages->isEmpty()) {
    mListWidget->addItem(tr("Looks good so far :-)"));
  }
  mListWidget->setEnabled(!mDisplayedMessages.isEmpty());
  blockSignals(signalsBlocked);
  mListWidget->setUpdatesEnabled(true);

  // Update count of unapproved messages.
  if (mMessages) {
    mUnapprovedMessageCount = unapprovedMessageCount;
  } else {
    mUnapprovedMessageCount = std::nullopt;
  }
}

void RuleCheckListWidget::currentItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  // Update item selection state to ensure proper theme-aware text color.
  if (RuleCheckListItemWidget* w = qobject_cast<RuleCheckListItemWidget*>(
          mListWidget->itemWidget(previous))) {
    w->setSelected(false);
  }
  if (RuleCheckListItemWidget* w = qobject_cast<RuleCheckListItemWidget*>(
          mListWidget->itemWidget(current))) {
    w->setSelected(true);
  }

  std::shared_ptr<const RuleCheckMessage> msg =
      mDisplayedMessages.value(mListWidget->row(current));
  if (msg && mHandler) {
    mHandler->ruleCheckMessageSelected(msg);
  }
}

void RuleCheckListWidget::itemDoubleClicked(QListWidgetItem* item) noexcept {
  std::shared_ptr<const RuleCheckMessage> msg =
      mDisplayedMessages.value(mListWidget->row(item));
  if (msg && mHandler) {
    mHandler->ruleCheckMessageDoubleClicked(msg);
  }
}

bool RuleCheckListWidget::ruleCheckFixAvailable(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if ((!mReadOnly) && mHandler) {
    return mHandler->ruleCheckFixAvailable(msg);
  } else {
    return false;
  }
}

void RuleCheckListWidget::ruleCheckFixRequested(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (mHandler) {
    mHandler->ruleCheckFixRequested(msg);
  }
}

void RuleCheckListWidget::ruleCheckDescriptionRequested(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (mHandler) {
    mHandler->ruleCheckDescriptionRequested(msg);
  }
}

void RuleCheckListWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
  if (mHandler) {
    mHandler->ruleCheckApproveRequested(msg, approve);
  }
}

void RuleCheckListWidget::ruleCheckMessageSelected(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (mHandler) {
    mHandler->ruleCheckMessageSelected(msg);
  }
}

void RuleCheckListWidget::ruleCheckMessageDoubleClicked(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (mHandler) {
    mHandler->ruleCheckMessageDoubleClicked(msg);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
