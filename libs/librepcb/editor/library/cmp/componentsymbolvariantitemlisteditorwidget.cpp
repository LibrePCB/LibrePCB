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
#include "componentsymbolvariantitemlisteditorwidget.h"

#include "../../library/cmp/componentsymbolvariantitemlistmodel.h"
#include "../../widgets/editabletablewidget.h"
#include "../sym/symbolchooserdialog.h"

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

ComponentSymbolVariantItemListEditorWidget::
    ComponentSymbolVariantItemListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new ComponentSymbolVariantItemListModel(this)),
    mView(new EditableTableWidget(this)),
    mWorkspace(nullptr),
    mLayerProvider(nullptr),
    mOnItemListEditedSlot(
        *this, &ComponentSymbolVariantItemListEditorWidget::itemListEdited),
    mOnItemEditedSlot(*this,
                      &ComponentSymbolVariantItemListEditorWidget::itemEdited) {
  mView->setShowMoveButtons(true);
  mView->setBrowseButtonColumn(
      ComponentSymbolVariantItemListModel::COLUMN_SYMBOL);
  mView->setModel(mModel.data());
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_NUMBER,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_SYMBOL, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_SUFFIX,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_ISREQUIRED,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_X,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_Y,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_ROTATION,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSymbolVariantItemListModel::COLUMN_ACTIONS,
      QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &ComponentSymbolVariantItemListModel::addItem);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &ComponentSymbolVariantItemListModel::removeItem);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &ComponentSymbolVariantItemListModel::moveItemUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &ComponentSymbolVariantItemListModel::moveItemDown);
  connect(mView.data(), &EditableTableWidget::btnBrowseClicked, this,
          &ComponentSymbolVariantItemListEditorWidget::btnSymbolBrowseClicked);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

ComponentSymbolVariantItemListEditorWidget::
    ~ComponentSymbolVariantItemListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSymbolVariantItemListEditorWidget::setReadOnly(
    bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void ComponentSymbolVariantItemListEditorWidget::setReferences(
    const Workspace& ws, const IF_GraphicsLayerProvider& layerProvider,
    ComponentSymbolVariantItemList& items,
    const std::shared_ptr<const LibraryElementCache>& symbolCache,
    UndoStack* undoStack) noexcept {
  mOnItemListEditedSlot.detachAll();
  mOnItemEditedSlot.detachAll();

  mWorkspace = &ws;
  mLayerProvider = &layerProvider;
  mModel->setSymbolsCache(symbolCache);
  mModel->setItemList(&items);
  mModel->setUndoStack(undoStack);

  items.onEdited.attach(mOnItemListEditedSlot);
  items.onElementEdited.attach(mOnItemEditedSlot);
}

void ComponentSymbolVariantItemListEditorWidget::resetReferences() noexcept {
  mOnItemListEditedSlot.detachAll();
  mOnItemEditedSlot.detachAll();

  mModel->setItemList(nullptr);
  mModel->setUndoStack(nullptr);
  mModel->setSymbolsCache(nullptr);
  mLayerProvider = nullptr;
  mWorkspace = nullptr;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentSymbolVariantItemListEditorWidget::itemListEdited(
    const ComponentSymbolVariantItemList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariantItem>& item,
    ComponentSymbolVariantItemList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(item);
  switch (event) {
    case ComponentSymbolVariantItemList::Event::ElementAdded:
    case ComponentSymbolVariantItemList::Event::ElementRemoved:
      emit edited();
      emit triggerGraphicsItemsUpdate();
      break;
    default:
      break;
  }
  emit edited();
}

void ComponentSymbolVariantItemListEditorWidget::itemEdited(
    const ComponentSymbolVariantItemList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariantItem>& item,
    ComponentSymbolVariantItem::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(item);
  switch (event) {
    case ComponentSymbolVariantItem::Event::SymbolPositionChanged:
    case ComponentSymbolVariantItem::Event::SymbolRotationChanged:
    case ComponentSymbolVariantItem::Event::SymbolUuidChanged:
      emit triggerGraphicsItemsUpdate();
      break;
    default:
      break;
  }
}

void ComponentSymbolVariantItemListEditorWidget::btnSymbolBrowseClicked(
    const QVariant& data) noexcept {
  SymbolChooserDialog dialog(*mWorkspace, *mLayerProvider, this);
  if ((dialog.exec() == QDialog::Accepted) && dialog.getSelectedSymbolUuid()) {
    mModel->changeSymbol(data, *dialog.getSelectedSymbolUuid());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
