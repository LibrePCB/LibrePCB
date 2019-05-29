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
#include "wsi_librarynormorder.h"

#include <librepcb/common/norms.h>

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

WSI_LibraryNormOrder::WSI_LibraryNormOrder(const SExpression& node)
  : WSI_Base() {
  if (const SExpression* child = node.tryGetChildByPath("library_norm_order")) {
    foreach (const SExpression& childchild, child->getChildren()) {
      mList.append(childchild.getValueOfFirstChild<QString>());
    }
  }
  mList.removeDuplicates();
  mList.removeAll("");
  mListTmp = mList;

  // create the QListWidget
  mListWidget.reset(new QListWidget());
  updateListWidgetItems();

  // create a QComboBox with all available norms
  mComboBox.reset(new QComboBox());
  mComboBox->setEditable(true);
  mComboBox->addItems(getAvailableNorms());
  mComboBox->clearEditText();

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
          &WSI_LibraryNormOrder::btnUpClicked);
  connect(mBtnDown.data(), &QToolButton::clicked, this,
          &WSI_LibraryNormOrder::btnDownClicked);
  connect(mBtnAdd.data(), &QToolButton::clicked, this,
          &WSI_LibraryNormOrder::btnAddClicked);
  connect(mBtnRemove.data(), &QToolButton::clicked, this,
          &WSI_LibraryNormOrder::btnRemoveClicked);

  // create the QWidget
  mWidget.reset(new QWidget());
  QVBoxLayout* outerLayout = new QVBoxLayout(mWidget.data());
  outerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->addWidget(mListWidget.data());
  QHBoxLayout* innerLayout = new QHBoxLayout();
  innerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->addLayout(innerLayout);
  innerLayout->addWidget(mComboBox.data());
  innerLayout->addWidget(mBtnAdd.data());
  innerLayout->addWidget(mBtnRemove.data());
  innerLayout->addWidget(mBtnUp.data());
  innerLayout->addWidget(mBtnDown.data());
}

WSI_LibraryNormOrder::~WSI_LibraryNormOrder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WSI_LibraryNormOrder::restoreDefault() noexcept {
  mListTmp.clear();
  updateListWidgetItems();
}

void WSI_LibraryNormOrder::apply() noexcept {
  mList = mListTmp;
}

void WSI_LibraryNormOrder::revert() noexcept {
  mListTmp = mList;
  updateListWidgetItems();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WSI_LibraryNormOrder::btnUpClicked() noexcept {
  int row = mListWidget->currentRow();
  if (row > 0) {
    mListTmp.move(row, row - 1);
    mListWidget->insertItem(row - 1, mListWidget->takeItem(row));
    mListWidget->setCurrentRow(row - 1);
  }
}

void WSI_LibraryNormOrder::btnDownClicked() noexcept {
  int row = mListWidget->currentRow();
  if ((row >= 0) && (row < mListWidget->count() - 1)) {
    mListTmp.move(row, row + 1);
    mListWidget->insertItem(row + 1, mListWidget->takeItem(row));
    mListWidget->setCurrentRow(row + 1);
  }
}

void WSI_LibraryNormOrder::btnAddClicked() noexcept {
  if (!mComboBox->currentText().isEmpty()) {
    if (!mListTmp.contains(mComboBox->currentText())) {
      mListTmp.append(mComboBox->currentText());
      updateListWidgetItems();
    }
  }
}

void WSI_LibraryNormOrder::btnRemoveClicked() noexcept {
  if (mListWidget->currentRow() >= 0) {
    mListTmp.removeAt(mListWidget->currentRow());
    delete mListWidget->item(mListWidget->currentRow());
  }
}

void WSI_LibraryNormOrder::updateListWidgetItems() noexcept {
  mListWidget->clear();
  mListWidget->addItems(mListTmp);
}

void WSI_LibraryNormOrder::serialize(SExpression& root) const {
  SExpression& child = root.appendList("library_norm_order", true);
  foreach (const QString& norm, mList) {
    child.appendChild("norm", norm, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
