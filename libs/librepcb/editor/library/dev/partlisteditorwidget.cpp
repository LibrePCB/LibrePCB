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
#include "partlisteditorwidget.h"

#include "../../widgets/editabletablewidget.h"
#include "partlistmodel.h"

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

PartListEditorWidget::PartListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new PartListModel(this)),
    mView(new EditableTableWidget(this)) {
  mView->setShowMoveButtons(true);
  mView->setShowCopyButton(true);
  mView->setModel(mModel.data());
  mView->horizontalHeader()->setSectionResizeMode(PartListModel::COLUMN_MPN,
                                                  QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      PartListModel::COLUMN_MANUFACTURER, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      PartListModel::COLUMN_ATTRIBUTES, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      PartListModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &PartListModel::add);
  connect(mView.data(), &EditableTableWidget::btnCopyClicked, mModel.data(),
          &PartListModel::copy);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &PartListModel::remove);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &PartListModel::moveUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &PartListModel::moveDown);
  connect(mView.data(), &EditableTableWidget::currentRowChanged, this,
          &PartListEditorWidget::currentItemChanged);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

PartListEditorWidget::~PartListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PartListEditorWidget::setFrameStyle(int style) noexcept {
  mView->setFrameStyle(style);
}

void PartListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void PartListEditorWidget::setInitialManufacturer(
    const SimpleString& value) noexcept {
  mModel->setInitialManufacturer(value);
}

void PartListEditorWidget::setReferences(UndoStack* undoStack,
                                         PartList* list) noexcept {
  mModel->setPartList(list);
  mModel->setUndoStack(undoStack);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
