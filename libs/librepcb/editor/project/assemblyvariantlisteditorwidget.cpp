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
#include "assemblyvariantlisteditorwidget.h"

#include "../widgets/editabletablewidget.h"
#include "assemblyvariantlistmodel.h"

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

AssemblyVariantListEditorWidget::AssemblyVariantListEditorWidget(
    QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new AssemblyVariantListModel(this)),
    mView(new EditableTableWidget(this)) {
  mView->setAddButtonOnLastRow(false);
  mView->setShowMoveButtons(true);
  mView->setShowCopyButton(true);
  mView->setMinimumRowCount(1);
  mView->setModel(mModel.data());
  mView->horizontalHeader()->setSectionResizeMode(
      AssemblyVariantListModel::COLUMN_NAME, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      AssemblyVariantListModel::COLUMN_DESCRIPTION, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      AssemblyVariantListModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnCopyClicked, mModel.data(),
          &AssemblyVariantListModel::copy);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &AssemblyVariantListModel::remove);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &AssemblyVariantListModel::moveUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &AssemblyVariantListModel::moveDown);
  connect(mView.data(), &EditableTableWidget::currentRowChanged, this,
          &AssemblyVariantListEditorWidget::currentItemChanged);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

AssemblyVariantListEditorWidget::~AssemblyVariantListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AssemblyVariantListEditorWidget::setFrameStyle(int style) noexcept {
  mView->setFrameStyle(style);
}

void AssemblyVariantListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void AssemblyVariantListEditorWidget::setReferences(UndoStack* undoStack,
                                                    Circuit* circuit) noexcept {
  mModel->setCircuit(circuit);
  mModel->setUndoStack(undoStack);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
