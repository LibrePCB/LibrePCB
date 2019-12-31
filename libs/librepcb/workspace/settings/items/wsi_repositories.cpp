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
#include "wsi_repositories.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WSI_Repositories::WSI_Repositories(const SExpression& node) : WSI_Base() {
  if (const SExpression* child = node.tryGetChildByPath("repositories")) {
    foreach (const SExpression& repo, child->getChildren()) {
      mUrls.append(repo.getChildByIndex(0).getValue<QUrl>());  // can throw
    }
  } else {
    mUrls.append(QUrl("https://api.librepcb.org"));
  }
  mUrlsTmp = mUrls;

  // create the QListWidget
  mListWidget.reset(new QListWidget());
  updateListWidgetItems();

  // create a QLineEdit
  mLineEdit.reset(new QLineEdit());
  mLineEdit->setMaxLength(255);

  // create all buttons
  mBtnUp.reset(new QToolButton());
  mBtnDown.reset(new QToolButton());
  mBtnAdd.reset(new QToolButton());
  mBtnRemove.reset(new QToolButton());
  mBtnUp->setArrowType(Qt::UpArrow);
  mBtnDown->setArrowType(Qt::DownArrow);
  mBtnAdd->setIcon(QIcon(":/img/actions/plus_2.png"));
  mBtnRemove->setIcon(QIcon(":/img/actions/minus.png"));
  connect(mBtnUp.data(), &QToolButton::clicked, this,
          &WSI_Repositories::btnUpClicked);
  connect(mBtnDown.data(), &QToolButton::clicked, this,
          &WSI_Repositories::btnDownClicked);
  connect(mBtnAdd.data(), &QToolButton::clicked, this,
          &WSI_Repositories::btnAddClicked);
  connect(mBtnRemove.data(), &QToolButton::clicked, this,
          &WSI_Repositories::btnRemoveClicked);

  // create the QWidget
  mWidget.reset(new QWidget());
  QVBoxLayout* outerLayout = new QVBoxLayout(mWidget.data());
  outerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->addWidget(mListWidget.data());
  QHBoxLayout* innerLayout = new QHBoxLayout();
  innerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->addLayout(innerLayout);
  innerLayout->addWidget(mLineEdit.data());
  innerLayout->addWidget(mBtnAdd.data());
  innerLayout->addWidget(mBtnRemove.data());
  innerLayout->addWidget(mBtnUp.data());
  innerLayout->addWidget(mBtnDown.data());
}

WSI_Repositories::~WSI_Repositories() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WSI_Repositories::restoreDefault() noexcept {
  mUrlsTmp.clear();
  mUrlsTmp.append(QUrl("https://api.librepcb.org"));
  updateListWidgetItems();
}

void WSI_Repositories::apply() noexcept {
  mUrls = mUrlsTmp;
}

void WSI_Repositories::revert() noexcept {
  mUrlsTmp = mUrls;
  updateListWidgetItems();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WSI_Repositories::btnUpClicked() noexcept {
  int row = mListWidget->currentRow();
  if (row > 0) {
    mUrlsTmp.move(row, row - 1);
    mListWidget->insertItem(row - 1, mListWidget->takeItem(row));
    mListWidget->setCurrentRow(row - 1);
  }
}

void WSI_Repositories::btnDownClicked() noexcept {
  int row = mListWidget->currentRow();
  if ((row >= 0) && (row < mListWidget->count() - 1)) {
    mUrlsTmp.move(row, row + 1);
    mListWidget->insertItem(row + 1, mListWidget->takeItem(row));
    mListWidget->setCurrentRow(row + 1);
  }
}

void WSI_Repositories::btnAddClicked() noexcept {
  QUrl url = QUrl::fromUserInput(mLineEdit->text().trimmed());
  if (url.isValid()) {
    mUrlsTmp.append(url);
    updateListWidgetItems();
    mLineEdit->setText("");
  } else {
    QMessageBox::critical(mWidget.data(), tr("Error"),
                          tr("The URL is not valid."));
  }
}

void WSI_Repositories::btnRemoveClicked() noexcept {
  if (mListWidget->currentRow() >= 0) {
    mUrlsTmp.removeAt(mListWidget->currentRow());
    delete mListWidget->item(mListWidget->currentRow());
  }
}

void WSI_Repositories::updateListWidgetItems() noexcept {
  mListWidget->clear();
  foreach (const QUrl& url, mUrlsTmp) {
    QListWidgetItem* item =
        new QListWidgetItem(url.toDisplayString(), mListWidget.data());
    item->setData(Qt::UserRole, url);
  }
}

void WSI_Repositories::serialize(SExpression& root) const {
  SExpression& child = root.appendList("repositories", true);
  foreach (const QUrl& url, mUrls) {
    child.appendChild("repository", url, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
