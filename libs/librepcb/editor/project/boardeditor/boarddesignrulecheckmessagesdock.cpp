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
#include "boarddesignrulecheckmessagesdock.h"

#include "ui_boarddesignrulecheckmessagesdock.h"

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

BoardDesignRuleCheckMessagesDock::BoardDesignRuleCheckMessagesDock(
    QWidget* parent) noexcept
  : QDockWidget(parent), mUi(new Ui::BoardDesignRuleCheckMessagesDock) {
  mUi->setupUi(this);
  mUi->prgProgress->setFixedHeight(mUi->cbxCenterInView->height());
  mUi->prgProgress->hide();
  mUi->btnSettings->setFixedSize(mUi->cbxCenterInView->height(),
                                 mUi->cbxCenterInView->height());
  mUi->btnRun->setFixedSize(mUi->cbxCenterInView->height(),
                            mUi->cbxCenterInView->height());
  connect(mUi->btnSettings, &QToolButton::clicked, this,
          &BoardDesignRuleCheckMessagesDock::settingsDialogRequested);
  connect(mUi->btnRun, &QToolButton::clicked, this,
          &BoardDesignRuleCheckMessagesDock::runDrcRequested);
  connect(mUi->lstMessages, &QListWidget::currentRowChanged, this,
          &BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemChanged);
  connect(mUi->lstMessages, &QListWidget::itemClicked, this,
          &BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemChanged);
  connect(
      mUi->lstMessages, &QListWidget::itemDoubleClicked, this,
      &BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemDoubleClicked);

  setInteractive(false);
}

BoardDesignRuleCheckMessagesDock::~BoardDesignRuleCheckMessagesDock() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool BoardDesignRuleCheckMessagesDock::setInteractive(
    bool interactive) noexcept {
  bool wasInteractive = mUi->btnRun->isEnabled();
  mUi->lstMessages->setEnabled(interactive);
  mUi->cbxCenterInView->setEnabled(interactive);
  mUi->btnSettings->setEnabled(interactive);
  mUi->btnRun->setEnabled(interactive);
  return wasInteractive;
}

void BoardDesignRuleCheckMessagesDock::setProgressPercent(
    int percent) noexcept {
  mUi->cbxCenterInView->hide();
  mUi->prgProgress->show();
  mUi->prgProgress->setValue(percent);
}

void BoardDesignRuleCheckMessagesDock::setProgressStatus(
    const QString& status) noexcept {
  mUi->cbxCenterInView->hide();
  mUi->prgProgress->show();
  mUi->prgProgress->setFormat(status);
}

void BoardDesignRuleCheckMessagesDock::setMessages(
    const QList<BoardDesignRuleCheckMessage>& messages) noexcept {
  mMessages = messages;

  mUi->prgProgress->hide();
  mUi->cbxCenterInView->show();

  bool signalsBlocked = mUi->lstMessages->blockSignals(true);
  mUi->lstMessages->clear();
  foreach (const BoardDesignRuleCheckMessage& message, mMessages) {
    mUi->lstMessages->addItem(message.getMessage());
  }
  mUi->lstMessages->blockSignals(signalsBlocked);

  setWindowTitle(tr("DRC [%1]", "Number of messages").arg(mMessages.count()));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemChanged() noexcept {
  int index = mUi->lstMessages->currentRow();
  if ((index >= 0) && (index < mMessages.count())) {
    emit messageSelected(mMessages[index], mUi->cbxCenterInView->isChecked());
  }
}

void BoardDesignRuleCheckMessagesDock::
    listWidgetCurrentItemDoubleClicked() noexcept {
  int index = mUi->lstMessages->currentRow();
  if ((index >= 0) && (index < mMessages.count())) {
    emit messageSelected(mMessages[index], true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
