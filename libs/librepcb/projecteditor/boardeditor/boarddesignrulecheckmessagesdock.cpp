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
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheckMessagesDock::BoardDesignRuleCheckMessagesDock(
    QWidget* parent) noexcept
  : QDockWidget(parent), mUi(new Ui::BoardDesignRuleCheckMessagesDock) {
  mUi->setupUi(this);
  connect(mUi->listWidget, &QListWidget::currentRowChanged, this,
          &BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemChanged);
  connect(mUi->listWidget, &QListWidget::itemClicked, this,
          &BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemChanged);
  connect(
      mUi->listWidget, &QListWidget::itemDoubleClicked, this,
      &BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemDoubleClicked);
}

BoardDesignRuleCheckMessagesDock::~BoardDesignRuleCheckMessagesDock() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void BoardDesignRuleCheckMessagesDock::setMessages(
    const QList<BoardDesignRuleCheckMessage>& messages) noexcept {
  mMessages = messages;

  bool signalsBlocked = mUi->listWidget->blockSignals(true);
  mUi->listWidget->clear();
  foreach (const BoardDesignRuleCheckMessage& message, mMessages) {
    mUi->listWidget->addItem(message.getMessage());
  }
  mUi->listWidget->blockSignals(signalsBlocked);

  setWindowTitle(
      QString(tr("DRC [%1]", "Number of messages")).arg(mMessages.count()));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardDesignRuleCheckMessagesDock::listWidgetCurrentItemChanged() noexcept {
  int index = mUi->listWidget->currentRow();
  if ((index >= 0) && (index < mMessages.count())) {
    emit messageSelected(mMessages[index], mUi->cbxCenterInView->isChecked());
  }
}

void BoardDesignRuleCheckMessagesDock::
    listWidgetCurrentItemDoubleClicked() noexcept {
  int index = mUi->listWidget->currentRow();
  if ((index >= 0) && (index < mMessages.count())) {
    emit messageSelected(mMessages[index], true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
