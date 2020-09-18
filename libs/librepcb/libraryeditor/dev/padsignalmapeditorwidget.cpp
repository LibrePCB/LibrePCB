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
#include "padsignalmapeditorwidget.h"

#include <librepcb/common/model/comboboxdelegate.h>
#include <librepcb/common/model/sortfilterproxymodel.h>
#include <librepcb/libraryeditor/dev/devicepadsignalmapmodel.h>

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

PadSignalMapEditorWidget::PadSignalMapEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new DevicePadSignalMapModel(this)),
    mProxy(new SortFilterProxyModel(this)),
    mView(new QTableView(this)) {
  mProxy->setSourceModel(mModel.data());
  mView->setModel(mProxy.data());
  mView->setAlternatingRowColors(true);  // increase readability
  mView->setCornerButtonEnabled(false);  // not needed
  mView->setSelectionBehavior(QAbstractItemView::SelectRows);
  mView->setSelectionMode(QAbstractItemView::SingleSelection);
  mView->setEditTriggers(QAbstractItemView::AllEditTriggers);
  mView->setSortingEnabled(true);
  mView->setWordWrap(false);  // avoid too high cells due to word wrap
  mView->verticalHeader()->setMinimumSectionSize(10);  // more compact rows
  mView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      DevicePadSignalMapModel::COLUMN_PAD, QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      DevicePadSignalMapModel::COLUMN_SIGNAL, QHeaderView::Stretch);
  mView->setItemDelegateForColumn(DevicePadSignalMapModel::COLUMN_SIGNAL,
                                  new ComboBoxDelegate(false, this));
  mView->sortByColumn(DevicePadSignalMapModel::COLUMN_PAD, Qt::AscendingOrder);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

PadSignalMapEditorWidget::~PadSignalMapEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PadSignalMapEditorWidget::setReferences(UndoStack* undoStack,
                                             DevicePadSignalMap* map) noexcept {
  mModel->setPadSignalMap(map);
  mModel->setUndoStack(undoStack);
}

void PadSignalMapEditorWidget::setPadList(const PackagePadList& list) noexcept {
  mModel->setPadList(list);
}

void PadSignalMapEditorWidget::setSignalList(
    const ComponentSignalList& list) noexcept {
  mModel->setSignalList(list);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
