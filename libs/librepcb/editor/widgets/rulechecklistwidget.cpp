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
    mHandler(nullptr),
    mApprovals(),
    mProvideFixes(true) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mListWidget.data());
  connect(mListWidget.data(), &QListWidget::itemDoubleClicked, this,
          &RuleCheckListWidget::itemDoubleClicked);
  updateList();  // adds the "looks good" message
}

RuleCheckListWidget::~RuleCheckListWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void RuleCheckListWidget::setProvideFixes(bool provideFixes) noexcept {
  if (provideFixes != mProvideFixes) {
    mProvideFixes = provideFixes;
    updateList();
  }
}

void RuleCheckListWidget::setHandler(IF_RuleCheckHandler* handler) noexcept {
  mHandler = handler;
}

void RuleCheckListWidget::setMessages(RuleCheckMessageList messages) noexcept {
  // Sort by severity and message.
  Toolbox::sortNumeric(messages,
                       [](const QCollator& cmp,
                          const std::shared_ptr<const RuleCheckMessage>& lhs,
                          const std::shared_ptr<const RuleCheckMessage>& rhs) {
                         if (lhs && rhs) {
                           if (lhs->getSeverity() != rhs->getSeverity()) {
                             return lhs->getSeverity() > rhs->getSeverity();
                           } else {
                             return cmp(lhs->getMessage(), rhs->getMessage());
                           }
                         } else {
                           return false;
                         }
                       },
                       Qt::CaseInsensitive, false);

  // Detect if messages have changed.
  bool isSame = (mMessages.count() == messages.count());
  if (isSame) {
    for (int i = 0; i < mMessages.count(); ++i) {
      auto m1 = mMessages.value(i);
      auto m2 = messages.value(i);
      if ((!m1) || (!m2) || (*m1 != *m2)) {
        isSame = false;
      }
    }
  }

  // Only update if messages have changed (avoid GUI flickering).
  if (!isSame) {
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
  mListWidget->setUpdatesEnabled(false);  // Avoid flicker.
  mListWidget->clear();
  foreach (const auto& msg, mMessages) {
    QListWidgetItem* item = new QListWidgetItem();
    mListWidget->addItem(item);
    const bool approved = mApprovals.contains(msg->getApproval());
    RuleCheckListItemWidget* widget =
        new RuleCheckListItemWidget(msg, *this, approved);
    mListWidget->setItemWidget(item, widget);
  }
  if (mListWidget->count() == 0) {
    mListWidget->setEnabled(false);
    mListWidget->addItem(tr("Looks good so far :-)"));
  } else {
    mListWidget->setEnabled(true);
  }
  mListWidget->setUpdatesEnabled(true);
}

void RuleCheckListWidget::itemDoubleClicked(QListWidgetItem* item) noexcept {
  std::shared_ptr<const RuleCheckMessage> msg =
      mMessages.value(mListWidget->row(item));
  if (msg && mHandler) {
    if (mProvideFixes && mHandler->ruleCheckFixAvailable(msg)) {
      mHandler->ruleCheckFixRequested(msg);
    } else {
      mHandler->ruleCheckDescriptionRequested(msg);
    }
  }
}

bool RuleCheckListWidget::ruleCheckFixAvailable(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (mProvideFixes && mHandler) {
    return mHandler->ruleCheckFixAvailable(msg);
  } else {
    return false;
  }
}

void RuleCheckListWidget::ruleCheckFixRequested(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (mProvideFixes && mHandler) {
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
