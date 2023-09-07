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
#include "packagemodellisteditorwidget.h"

#include "../../library/pkg/packagemodellistmodel.h"
#include "../../modelview/sortfilterproxymodel.h"
#include "../../widgets/editabletablewidget.h"

#include <librepcb/core/library/pkg/package.h>

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

PackageModelListEditorWidget::PackageModelListEditorWidget(
    QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new PackageModelListModel(this)),
    mProxy(new SortFilterProxyModel(this)),
    mView(new EditableTableWidget(this)) {
  mProxy->setKeepLastRowAtBottom(true);
  mProxy->setSourceModel(mModel.data());
  mView->setShowEditButton(true);
  mView->setShowMoveButtons(true);
  mView->setModel(mProxy.data());
  mView->horizontalHeader()->setSectionResizeMode(
      PackageModelListModel::COLUMN_ENABLED, QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      PackageModelListModel::COLUMN_NAME, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      PackageModelListModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &PackageModelListModel::add);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &PackageModelListModel::remove);
  connect(mView.data(), &EditableTableWidget::btnEditClicked, mModel.data(),
          &PackageModelListModel::edit);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &PackageModelListModel::moveUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &PackageModelListModel::moveDown);
  connect(mView.data(), &EditableTableWidget::currentRowChanged, this,
          &PackageModelListEditorWidget::currentIndexChanged);

  // Select model after adding a new one as the user expects that.
  connect(mModel.data(), &PackageModelListModel::newModelAdded, mView.data(),
          &EditableTableWidget::selectRow);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());

  setCurrentFootprint(nullptr);
}

PackageModelListEditorWidget::~PackageModelListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackageModelListEditorWidget::setFrameStyle(int style) noexcept {
  mView->setFrameStyle(style);
}

void PackageModelListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void PackageModelListEditorWidget::setReferences(Package* package,
                                                 UndoStack* stack) noexcept {
  mCurrentPackage = package;
  mModel->setPackage(package);
  mModel->setUndoStack(stack);
}

void PackageModelListEditorWidget::setCurrentFootprint(
    std::shared_ptr<Footprint> footprint) noexcept {
  mModel->setFootprint(footprint);
  mView->horizontalHeader()->setSectionHidden(
      PackageModelListModel::COLUMN_ENABLED, !footprint);

  // Switch selected model if none is selected or it is not compatible with the
  // footprint.
  if (mCurrentPackage) {
    const auto currentModel =
        mCurrentPackage->getModels().value(mView->currentIndex().row());
    const QSet<Uuid> compatibleModels =
        footprint ? footprint->getModels() : QSet<Uuid>();
    if ((!currentModel) ||
        (currentModel &&
         (!compatibleModels.contains(currentModel->getUuid())))) {
      for (int i = 0; i < mCurrentPackage->getModels().count(); ++i) {
        if (compatibleModels.contains(
                mCurrentPackage->getModels().at(i)->getUuid())) {
          mView->selectRow(i);
          return;
        }
      }
      mView->selectRow(-1);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
