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
#include "attributelisteditorwidget.h"

#include "../modelview/attributelistmodellegacy.h"
#include "../modelview/comboboxdelegate.h"
#include "../widgets/editabletablewidget.h"

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

AttributeListEditorWidget::AttributeListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new AttributeListModelLegacy(this)),
    mView(new EditableTableWidget(this)) {
  mView->setShowMoveButtons(true);
  mView->setModel(mModel.data());
  // Start editing with a single click to immediately show the comboboxes - not
  // very nice since edit triggers also apply to normal text cells, but better
  // than needing one more click to drop down comboboxes...
  mView->setEditTriggers(QAbstractItemView::AllEditTriggers);
  mView->horizontalHeader()->setSectionResizeMode(
      AttributeListModelLegacy::COLUMN_KEY, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      AttributeListModelLegacy::COLUMN_TYPE, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      AttributeListModelLegacy::COLUMN_VALUE, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      AttributeListModelLegacy::COLUMN_UNIT, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      AttributeListModelLegacy::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  mView->setItemDelegateForColumn(AttributeListModelLegacy::COLUMN_TYPE,
                                  new ComboBoxDelegate(false, this));
  mView->setItemDelegateForColumn(AttributeListModelLegacy::COLUMN_UNIT,
                                  new ComboBoxDelegate(false, this));
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &AttributeListModelLegacy::add);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &AttributeListModelLegacy::remove);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &AttributeListModelLegacy::moveUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &AttributeListModelLegacy::moveDown);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

AttributeListEditorWidget::~AttributeListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AttributeListEditorWidget::setFrameStyle(int style) noexcept {
  mView->setFrameStyle(style);
}

void AttributeListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void AttributeListEditorWidget::setReferences(UndoStack* undoStack,
                                              AttributeList* list) noexcept {
  mModel->setAttributeList(list);
  mModel->setUndoStack(undoStack);
  mView->horizontalHeader()->reset();  // Fix wrong column sizes.
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
