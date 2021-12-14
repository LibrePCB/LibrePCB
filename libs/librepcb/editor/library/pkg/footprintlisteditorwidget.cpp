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
#include "footprintlisteditorwidget.h"

#include <librepcb/common/widgets/editabletablewidget.h>
#include <librepcb/libraryeditor/pkg/footprintlistmodel.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintListEditorWidget::FootprintListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new FootprintListModel(this)),
    mView(new EditableTableWidget(this)) {
  mView->setShowCopyButton(true);
  mView->setShowMoveButtons(true);
  mView->setModel(mModel.data());
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_NAME, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &FootprintListModel::addFootprint);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &FootprintListModel::removeFootprint);
  connect(mView.data(), &EditableTableWidget::btnCopyClicked, mModel.data(),
          &FootprintListModel::copyFootprint);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &FootprintListModel::moveFootprintUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &FootprintListModel::moveFootprintDown);
  connect(mView.data(), &EditableTableWidget::currentRowChanged, this,
          &FootprintListEditorWidget::currentFootprintChanged);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

FootprintListEditorWidget::~FootprintListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void FootprintListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void FootprintListEditorWidget::setReferences(FootprintList& list,
                                              UndoStack& stack) noexcept {
  mModel->setFootprintList(&list);
  mModel->setUndoStack(&stack);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
