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
#include "compsymbvarpinsignalmapeditorwidget.h"

#include "../../library/cmp/componentpinsignalmapmodel.h"
#include "../../modelview/comboboxdelegate.h"
#include "../../modelview/sortfilterproxymodel.h"

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

CompSymbVarPinSignalMapEditorWidget::CompSymbVarPinSignalMapEditorWidget(
    QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new ComponentPinSignalMapModel(this)),
    mProxy(new SortFilterProxyModel(this)),
    mView(new QTableView(this)),
    mBtnAutoAssign(
        new QPushButton(tr("Automatically assign all signals by name"), this)) {
  mProxy->setSourceModel(mModel.data());
  mView->setModel(mProxy.data());
  mView->setAlternatingRowColors(true);  // increase readability
  mView->setCornerButtonEnabled(false);  // not needed
  mView->setSelectionBehavior(QAbstractItemView::SelectRows);
  mView->setSelectionMode(QAbstractItemView::SingleSelection);
  mView->setEditTriggers(QAbstractItemView::AllEditTriggers);
  mView->setSortingEnabled(true);
  mView->setWordWrap(false);  // avoid too high cells due to word wrap
  mView->verticalHeader()->setVisible(false);  // no content
  mView->verticalHeader()->setMinimumSectionSize(10);  // more compact rows
  mView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentPinSignalMapModel::COLUMN_SYMBOL, QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentPinSignalMapModel::COLUMN_PIN, QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentPinSignalMapModel::COLUMN_SIGNAL, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentPinSignalMapModel::COLUMN_DISPLAY, QHeaderView::Stretch);
  mView->setItemDelegateForColumn(ComponentPinSignalMapModel::COLUMN_SIGNAL,
                                  new ComboBoxDelegate(false, this));
  mView->setItemDelegateForColumn(ComponentPinSignalMapModel::COLUMN_DISPLAY,
                                  new ComboBoxDelegate(false, this));
  mView->sortByColumn(ComponentPinSignalMapModel::COLUMN_PIN,
                      Qt::AscendingOrder);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(3);
  layout->addWidget(mView.data());
  connect(mBtnAutoAssign.data(), &QPushButton::clicked, mModel.data(),
          &ComponentPinSignalMapModel::autoAssignSignals);
  layout->addWidget(mBtnAutoAssign.data());
}

CompSymbVarPinSignalMapEditorWidget::
    ~CompSymbVarPinSignalMapEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CompSymbVarPinSignalMapEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setEditTriggers(readOnly
                             ? QAbstractItemView::EditTrigger::NoEditTriggers
                             : QAbstractItemView::AllEditTriggers);
  mBtnAutoAssign->setHidden(readOnly);
}

void CompSymbVarPinSignalMapEditorWidget::setReferences(
    ComponentSymbolVariant* variant,
    const std::shared_ptr<const LibraryElementCache>& symbolCache,
    const ComponentSignalList* sigs, UndoStack* undoStack) noexcept {
  mModel->setUndoStack(undoStack);
  mModel->setSymbolsCache(symbolCache);
  mModel->setSignalList(sigs);
  mModel->setSymbolVariant(variant);
}

void CompSymbVarPinSignalMapEditorWidget::resetReferences() noexcept {
  mModel->setSymbolVariant(nullptr);
  mModel->setSignalList(nullptr);
  mModel->setUndoStack(nullptr);
  mModel->setSymbolsCache(nullptr);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
