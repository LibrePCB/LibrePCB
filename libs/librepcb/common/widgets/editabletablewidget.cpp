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
#include "editabletablewidget.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EditableTableWidget::EditableTableWidget(QWidget* parent) noexcept
  : QTableView(parent),
    mShowCopyButton(false),
    mShowEditButton(false),
    mShowMoveButtons(false),
    mBrowseButtonColumn(-1),
    mReadOnly(false) {
  // set reasonable default values - they can still be changed afterwards
  setAlternatingRowColors(true);  // increase readability
  setCornerButtonEnabled(false);  // not needed
  setSelectionBehavior(QAbstractItemView::SelectRows);  // our default style
  setSelectionMode(QAbstractItemView::SingleSelection);  // our default style
  setSortingEnabled(false);  // avoid too wide last column (no indicator)
  setWordWrap(false);  // avoid too high cells due to word wrap
  horizontalHeader()->setMinimumSectionSize(5);  // for button columns
  verticalHeader()->setMinimumSectionSize(10);  // more compact rows
  verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

EditableTableWidget::~EditableTableWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void EditableTableWidget::setReadOnly(bool readOnly) noexcept {
  if (readOnly != mReadOnly) {
    mReadOnly = readOnly;
    emit readOnlyChanged(readOnly);
  }
}

/*******************************************************************************
 *  Inherited
 ******************************************************************************/

void EditableTableWidget::reset() {
  QTableView::reset();
  if (model()) {
    for (int i = 0; i < model()->rowCount(); ++i) {
      installButtons(i);
    }
  }
}

bool EditableTableWidget::edit(const QModelIndex& index, EditTrigger trigger,
                               QEvent* event) {
  if (mReadOnly) {
    return false;
  } else {
    return QTableView::edit(index, trigger, event);
  }
}

void EditableTableWidget::currentChanged(const QModelIndex& current,
                                         const QModelIndex& previous) {
  QTableView::currentChanged(current, previous);
  if (current.isValid() &&
      (!previous.isValid() || (previous.row() != current.row()))) {
    emit currentRowChanged(current.row());
  }
}

void EditableTableWidget::rowsInserted(const QModelIndex& parent, int start,
                                       int end) {
  Q_UNUSED(parent);
  for (int i = start; i <= end; ++i) {
    installButtons(i);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EditableTableWidget::installButtons(int row) noexcept {
  if (!model()) {
    return;
  }

  // browse button column
  if (mBrowseButtonColumn >= 0) {
    QModelIndex index = model()->index(row, mBrowseButtonColumn);
    if (!indexWidget(index)) {
      QWidget* widget = new QWidget();
      widget->setContentsMargins(0, 0, 0, 0);
      QHBoxLayout* layout = new QHBoxLayout(widget);
      layout->setContentsMargins(0, 0, 0, 0);
      layout->setSpacing(0);
      layout->addStretch(1);
      int size = rowHeight(row);
      layout->addWidget(
          createButton("btnBrowse", QIcon(), "...", tr("Browse"), size, size,
                       &EditableTableWidget::btnBrowseClicked, index, true));
      setIndexWidget(index, widget);
    }
  }

  // last column
  QModelIndex index = model()->index(row, model()->columnCount() - 1);
  if (!indexWidget(index)) {
    QWidget* widget = new QWidget();
    widget->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    int size = rowHeight(row);
    if (row < model()->rowCount() - 1) {
      if (mShowEditButton) {
        layout->addWidget(createButton(
            "btnEdit", QIcon(":/img/actions/edit.png"), "", tr("Edit"), size,
            size, &EditableTableWidget::btnEditClicked, index, false));
      }
      if (mShowCopyButton) {
        layout->addWidget(createButton(
            "btnCopy", QIcon(":/img/actions/copy.png"), "", tr("Copy"), size,
            size, &EditableTableWidget::btnCopyClicked, index, true));
      }
      if (mShowMoveButtons) {
        layout->addWidget(createButton(
            "btnMoveUp", QIcon(":/img/actions/up.png"), "", tr("Move up"), size,
            size, &EditableTableWidget::btnMoveUpClicked, index, true));
        layout->addWidget(createButton(
            "btnMoveDown", QIcon(":/img/actions/down.png"), "", tr("Move down"),
            size, size, &EditableTableWidget::btnMoveDownClicked, index, true));
      }
      layout->addWidget(createButton(
          "btnRemove", QIcon(":/img/actions/minus.png"), "", tr("Remove"), size,
          size, &EditableTableWidget::btnRemoveClicked, index, true));
    } else {
      int width = size;
      if (mShowEditButton) width += size;
      if (mShowCopyButton) width += size;
      if (mShowMoveButtons) width += 2 * size;
      layout->addWidget(createButton(
          "btnAdd", QIcon(":/img/actions/add.png"), "", tr("Add"), width, size,
          &EditableTableWidget::btnAddClicked, index, true));
    }
    setIndexWidget(index, widget);
  }
}

QToolButton* EditableTableWidget::createButton(
    const QString& objectName, const QIcon& icon, const QString& text,
    const QString& toolTip, int width, int height, Signal clickedSignal,
    const QPersistentModelIndex& index, bool doesModify) noexcept {
  QToolButton* btn = new QToolButton();
  btn->setObjectName(objectName);
  btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  btn->setFixedSize(width, height);
  btn->setText(text);
  btn->setIcon(icon);
  btn->setIconSize(QSize(height - 4, height - 4));
  btn->setToolTip(toolTip);
  if (doesModify) {
    btn->setDisabled(mReadOnly);
    connect(this, &EditableTableWidget::readOnlyChanged, btn,
            &QPushButton::setDisabled);
  }
  connect(btn, &QToolButton::clicked, this, [this, index, clickedSignal]() {
    buttonClickedHandler(clickedSignal, index);
  });
  return btn;
}

void EditableTableWidget::buttonClickedHandler(
    Signal clickedSignal, const QPersistentModelIndex& index) noexcept {
  if (clickedSignal && index.isValid()) {
    QVariant data = index.data(Qt::EditRole);
    (this->*clickedSignal)(data);
  } else {
    qCritical() << "Invalid index received in "
                   "EditableTableWidget::buttonClickedHandler()";
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
