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
#include "componentsymbolvariantlistwidget.h"

#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/editabletablewidget.h>
#include <librepcb/library/cmp/cmd/cmdcomponentsymbolvariantedit.h>
#include <librepcb/libraryeditor/cmp/componentsymbolvariantlistmodel.h>

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

ComponentSymbolVariantListWidget::ComponentSymbolVariantListWidget(
    QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new ComponentSymbolVariantListModel(this)),
    mView(new EditableTableWidget(this)),
    mSymbolVariantList(nullptr),
    mUndoStack(nullptr),
    mEditorProvider(nullptr) {
  mView->setShowEditButton(true);
  mView->setShowMoveButtons(true);
  // don't use double click as edit trigger because it opens the dialog!
  mView->setEditTriggers(QAbstractItemView::EditKeyPressed |
                         QAbstractItemView::AnyKeyPressed);
  mView->setModel(mModel.data());
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantListModel::COLUMN_NAME, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantListModel::COLUMN_DESCRIPTION,
      QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantListModel::COLUMN_NORM, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantListModel::COLUMN_SYMBOLCOUNT,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantListModel::COLUMN_ACTIONS,
      QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &ComponentSymbolVariantListModel::addSymbolVariant);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &ComponentSymbolVariantListModel::removeSymbolVariant);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &ComponentSymbolVariantListModel::moveSymbolVariantUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &ComponentSymbolVariantListModel::moveSymbolVariantDown);
  connect(mView.data(), &EditableTableWidget::btnEditClicked, this,
          &ComponentSymbolVariantListWidget::btnEditClicked);
  connect(mView.data(), &EditableTableWidget::doubleClicked, this,
          &ComponentSymbolVariantListWidget::viewDoubleClicked);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

ComponentSymbolVariantListWidget::~ComponentSymbolVariantListWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSymbolVariantListWidget::setReferences(
    UndoStack* undoStack, ComponentSymbolVariantList* list,
    IF_ComponentSymbolVariantEditorProvider* editorProvider) noexcept {
  mModel->setSymbolVariantList(list);
  mModel->setUndoStack(undoStack);
  mSymbolVariantList = list;
  mUndoStack         = undoStack;
  mEditorProvider    = editorProvider;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentSymbolVariantListWidget::btnEditClicked(
    const QVariant& data) noexcept {
  tl::optional<Uuid> uuid = Uuid::tryFromString(data.toString());
  if (uuid && mSymbolVariantList && mUndoStack && mEditorProvider) {
    editVariant(*uuid);
  }
}

void ComponentSymbolVariantListWidget::viewDoubleClicked(
    const QModelIndex& index) noexcept {
  if (index.isValid() && mSymbolVariantList && mUndoStack && mEditorProvider) {
    auto variant = mSymbolVariantList->value(index.row());
    if (variant) {
      editVariant(variant->getUuid());
    } else {
      mView->edit(index);
    }
  }
}

void ComponentSymbolVariantListWidget::editVariant(const Uuid& uuid) noexcept {
  try {
    auto                   variant = mSymbolVariantList->get(uuid);
    ComponentSymbolVariant copy(*variant);
    if (mEditorProvider->openComponentSymbolVariantEditor(copy)) {
      QScopedPointer<CmdComponentSymbolVariantEdit> cmd(
          new CmdComponentSymbolVariantEdit(*variant));
      cmd->setNorm(copy.getNorm());
      cmd->setNames(copy.getNames());
      cmd->setDescriptions(copy.getDescriptions());
      cmd->setSymbolItems(copy.getSymbolItems());
      mUndoStack->execCmd(cmd.take());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not edit symbol variant"),
                          e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
