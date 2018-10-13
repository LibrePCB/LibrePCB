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

#include <librepcb/common/network/repository.h>

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
      mList.append(new Repository(repo));  // can throw
    }
  } else {
    mList.append(new Repository(QUrl("https://api.librepcb.org")));
  }
  foreach (const Repository* repository, mList) {
    mListTmp.append(new Repository(*repository));
  }

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
  qDeleteAll(mList);
  mList.clear();
  qDeleteAll(mListTmp);
  mListTmp.clear();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WSI_Repositories::restoreDefault() noexcept {
  qDeleteAll(mListTmp);
  mListTmp.clear();
  mListTmp.append(new Repository(QUrl("https://api.librepcb.org")));
  updateListWidgetItems();
}

void WSI_Repositories::apply() noexcept {
  qDeleteAll(mList);
  mList.clear();
  foreach (const Repository* repository, mListTmp) {
    mList.append(new Repository(*repository));
  }
}

void WSI_Repositories::revert() noexcept {
  qDeleteAll(mListTmp);
  mListTmp.clear();
  foreach (const Repository* repository, mList) {
    mListTmp.append(new Repository(*repository));
  }
  updateListWidgetItems();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WSI_Repositories::btnUpClicked() noexcept {
  int row = mListWidget->currentRow();
  if (row > 0) {
    mListTmp.move(row, row - 1);
    mListWidget->insertItem(row - 1, mListWidget->takeItem(row));
    mListWidget->setCurrentRow(row - 1);
  }
}

void WSI_Repositories::btnDownClicked() noexcept {
  int row = mListWidget->currentRow();
  if ((row >= 0) && (row < mListWidget->count() - 1)) {
    mListTmp.move(row, row + 1);
    mListWidget->insertItem(row + 1, mListWidget->takeItem(row));
    mListWidget->setCurrentRow(row + 1);
  }
}

void WSI_Repositories::btnAddClicked() noexcept {
  QUrl url = QUrl::fromUserInput(mLineEdit->text().trimmed());
  QScopedPointer<Repository> repository(new Repository(url));
  if (repository->isValid()) {
    mListTmp.append(repository.take());
    updateListWidgetItems();
    mLineEdit->setText("");
  } else {
    QMessageBox::critical(mWidget.data(), tr("Error"),
                          tr("The URL is not valid."));
  }
}

void WSI_Repositories::btnRemoveClicked() noexcept {
  if (mListWidget->currentRow() >= 0) {
    delete mListTmp.takeAt(mListWidget->currentRow());
    delete mListWidget->item(mListWidget->currentRow());
  }
}

void WSI_Repositories::updateListWidgetItems() noexcept {
  mListWidget->clear();
  foreach (const Repository* repository, mListTmp) {
    QString          str  = repository->getUrl().toDisplayString();
    QListWidgetItem* item = new QListWidgetItem(str, mListWidget.data());
    item->setData(Qt::UserRole, repository);
  }
}

void WSI_Repositories::serialize(SExpression& root) const {
  SExpression& child = root.appendList("repositories", true);
  serializePointerContainer(child, mList, "repository");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
