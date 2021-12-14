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
#include "componentsignallisteditorwidget.h"

#include "../../library/cmp/componentsignallistmodel.h"
#include "../../modelview/sortfilterproxymodel.h"
#include "../../widgets/editabletablewidget.h"

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

ComponentSignalListEditorWidget::ComponentSignalListEditorWidget(
    QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new ComponentSignalListModel(this)),
    mProxy(new SortFilterProxyModel(this)),
    mView(new EditableTableWidget(this)) {
  mProxy->setKeepLastRowAtBottom(true);
  mProxy->setSourceModel(mModel.data());
  mView->setModel(mProxy.data());
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSignalListModel::COLUMN_NAME, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSignalListModel::COLUMN_ISREQUIRED,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSignalListModel::COLUMN_FORCEDNETNAME, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      ComponentSignalListModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  mView->sortByColumn(ComponentSignalListModel::COLUMN_NAME,
                      Qt::AscendingOrder);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &ComponentSignalListModel::addSignal);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &ComponentSignalListModel::removeSignal);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

ComponentSignalListEditorWidget::~ComponentSignalListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSignalListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void ComponentSignalListEditorWidget::setReferences(
    UndoStack* undoStack, ComponentSignalList* list) noexcept {
  mModel->setSignalList(list);
  mModel->setUndoStack(undoStack);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
