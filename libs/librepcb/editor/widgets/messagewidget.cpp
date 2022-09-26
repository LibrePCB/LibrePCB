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
#include "messagewidget.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MessageWidget::MessageWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mLayout(new QHBoxLayout()),
    mMessageLabel(new QLabel()),
    mDismissLabel(new QLabel()),
    mHideLabel(new QLabel()),
    mWorkspace(),
    mDismissKey(),
    mActive(true),
    mTemporarilyHidden(false) {
  setAttribute(Qt::WA_StyledBackground, true);
  setStyleSheet("background-color: rgb(255, 255, 127);");
  mLayout->setContentsMargins(9, 5, 9, 5);
  setLayout(mLayout);

  // Setup message label.
  QFont font = mMessageLabel->font();
  font.setBold(true);
  mMessageLabel->setFont(font);
  mMessageLabel->setStyleSheet("color: rgb(170, 0, 0);");
  mMessageLabel->setWordWrap(true);
  connect(mMessageLabel.data(), &QLabel::linkActivated, this,
          &MessageWidget::linkActivated);
  mLayout->addWidget(mMessageLabel.data());

  // Setup "don't show again" label.
  mDismissLabel->setText("<small><a href='x'>" % tr("Don't show again") %
                         "</a></small>");
  mDismissLabel->setToolTip(
      tr("Permanently hide this message.\n"
         "This can be reverted in the workspace settings dialog."));
  mDismissLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  mDismissLabel->setIndent(7);
  connect(mDismissLabel.data(), &QLabel::linkActivated, this, [this]() {
    try {
      if (mWorkspace && (!mDismissKey.isEmpty())) {
        mWorkspace->getSettings().dismissedMessages.add(mDismissKey);
        mWorkspace->saveSettings();  // can throw
        updateVisibility();
      }
    } catch (const Exception& e) {
      qCritical().noquote() << "Failed to dismiss message:" << e.getMsg();
    }
  });
  mLayout->addWidget(mDismissLabel.data());

  // Setup "hide" label.
  mHideLabel->setText(
      "<h3><a href='x' style='text-decoration:none;'>✖</a></h3>");
  mHideLabel->setToolTip(tr("Temporarily hide this message."));
  mHideLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  mHideLabel->setIndent(4);
  connect(mHideLabel.data(), &QLabel::linkActivated, this, [this]() {
    mTemporarilyHidden = true;
    updateVisibility();
  });
  mLayout->addWidget(mHideLabel.data());
}

MessageWidget::~MessageWidget() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void MessageWidget::init(const QString& message, bool active) noexcept {
  setWorkspace(nullptr);
  mDismissKey.clear();
  mMessageLabel->setText(message);
  mDismissLabel->hide();
  setActive(active);
}

void MessageWidget::init(Workspace& workspace, const QString& dismissKey,
                         const QString& message, bool active) noexcept {
  setWorkspace(&workspace);
  mDismissKey = dismissKey;
  mMessageLabel->setText(message);
  mDismissLabel->show();
  setActive(active);
}

void MessageWidget::setActive(bool active) noexcept {
  mActive = active;
  if (!active) {
    mTemporarilyHidden = false;
  }
  updateVisibility();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void MessageWidget::setWorkspace(Workspace* workspace) noexcept {
  if (mWorkspace) {
    disconnect(&mWorkspace->getSettings().dismissedMessages,
               &WorkspaceSettingsItem::edited, this,
               &MessageWidget::updateVisibility);
  }
  mWorkspace = workspace;
  if (mWorkspace) {
    connect(&mWorkspace->getSettings().dismissedMessages,
            &WorkspaceSettingsItem::edited, this,
            &MessageWidget::updateVisibility);
  }
}

void MessageWidget::updateVisibility() noexcept {
  bool isVisible = mActive && (!mTemporarilyHidden);
  if (isVisible && mWorkspace && (!mDismissKey.isEmpty()) &&
      mWorkspace->getSettings().dismissedMessages.contains(mDismissKey)) {
    isVisible = false;
  }
  setVisible(isVisible);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
