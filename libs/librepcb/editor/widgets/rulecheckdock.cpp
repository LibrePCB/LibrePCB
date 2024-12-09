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
#include "rulecheckdock.h"

#include "ui_rulecheckdock.h"

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

RuleCheckDock::RuleCheckDock(Mode mode, QWidget* parent) noexcept
  : QDockWidget(parent), mMode(mode), mUi(new Ui::RuleCheckDock) {
  mUi->setupUi(this);
  updateTitle(std::nullopt);
  mUi->lstMessages->setHandler(this);
  mUi->cbxCenterInView->setVisible(mode == Mode::BoardDesignRuleCheck);
  mUi->prgProgress->hide();
  mUi->btnSettings->setVisible(mode == Mode::BoardDesignRuleCheck);
  mUi->btnRunDrc->setVisible(mode == Mode::BoardDesignRuleCheck);
  mUi->btnRunQuickCheck->setVisible(mode == Mode::BoardDesignRuleCheck);
  connect(mUi->btnSettings, &QToolButton::clicked, this,
          &RuleCheckDock::settingsDialogRequested);
  connect(mUi->btnRunDrc, &QToolButton::clicked, this,
          &RuleCheckDock::runDrcRequested);
  connect(mUi->btnRunQuickCheck, &QToolButton::clicked, this,
          &RuleCheckDock::runQuickCheckRequested);
}

RuleCheckDock::~RuleCheckDock() noexcept {
  mUi->lstMessages->setHandler(nullptr);
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool RuleCheckDock::setInteractive(bool interactive) noexcept {
  bool wasInteractive = mUi->btnRunDrc->isEnabled();
  mUi->lstMessages->setEnabled(interactive);
  mUi->cbxCenterInView->setEnabled(interactive);
  mUi->btnSettings->setEnabled(interactive);
  mUi->btnRunDrc->setEnabled(interactive);
  mUi->btnRunQuickCheck->setEnabled(interactive);
  return wasInteractive;
}

void RuleCheckDock::setProgressPercent(int percent) noexcept {
  if (mMode == Mode::BoardDesignRuleCheck) {
    mUi->cbxCenterInView->hide();
  }
  mUi->prgProgress->show();
  mUi->prgProgress->setValue(percent);
}

void RuleCheckDock::setProgressStatus(const QString& status) noexcept {
  if (mMode == Mode::BoardDesignRuleCheck) {
    mUi->cbxCenterInView->hide();
  }
  mUi->prgProgress->show();
  mUi->prgProgress->setFormat(status);
}

void RuleCheckDock::setMessages(
    const std::optional<RuleCheckMessageList>& messages) noexcept {
  mUi->prgProgress->hide();
  mUi->prgProgress->setValue(0);
  mUi->prgProgress->setFormat(QString());
  if (mMode == Mode::BoardDesignRuleCheck) {
    mUi->cbxCenterInView->show();
  }
  mUi->lstMessages->setMessages(messages);
  updateTitle(mUi->lstMessages->getUnapprovedMessageCount());
}

void RuleCheckDock::setApprovals(const QSet<SExpression>& approvals) noexcept {
  mUi->lstMessages->setApprovals(approvals);
  updateTitle(mUi->lstMessages->getUnapprovedMessageCount());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RuleCheckDock::updateTitle(
    std::optional<int> unapprovedMessages) noexcept {
  QString title;
  switch (mMode) {
    case Mode::ElectricalRuleCheck:
      title = tr("ERC");
      break;
    case Mode::BoardDesignRuleCheck:
      title = tr("DRC");
      break;
    default:
      qCritical() << "Unknown rule check dock mode.";
      break;
  }
  if (unapprovedMessages) {
    title += QString(" [%1]").arg(*unapprovedMessages);
  }
  setWindowTitle(title);
}

bool RuleCheckDock::ruleCheckFixAvailable(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  Q_UNUSED(msg);
  return false;
}

void RuleCheckDock::ruleCheckFixRequested(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  Q_UNUSED(msg);
}

void RuleCheckDock::ruleCheckDescriptionRequested(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (msg) {
    QMessageBox::information(this, msg->getMessage(), msg->getDescription());
  }
}

void RuleCheckDock::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
  if (msg) {
    emit messageApprovalRequested(*msg, approve);
  }
}

void RuleCheckDock::ruleCheckMessageSelected(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (msg) {
    emit messageSelected(*msg, mUi->cbxCenterInView->isChecked());
  }
}

void RuleCheckDock::ruleCheckMessageDoubleClicked(
    std::shared_ptr<const RuleCheckMessage> msg) noexcept {
  if (msg) {
    if (mUi->cbxCenterInView->isChecked()) {
      ruleCheckDescriptionRequested(msg);
    } else {
      emit messageSelected(*msg, true);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
