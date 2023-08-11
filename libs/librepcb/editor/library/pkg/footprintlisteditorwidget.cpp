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

#include "../../library/pkg/footprintlistmodel.h"
#include "../../modelview/angledelegate.h"
#include "../../modelview/lengthdelegate.h"
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

FootprintListEditorWidget::FootprintListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new FootprintListModel(this)),
    mView(new EditableTableWidget(this)),
    mLengthDelegateX(new LengthDelegate(this)),
    mLengthDelegateY(new LengthDelegate(this)),
    mLengthDelegateZ(new LengthDelegate(this)) {
  mView->setShowCopyButton(true);
  mView->setShowMoveButtons(true);
  mView->setModel(mModel.data());
  mView->setItemDelegateForColumn(FootprintListModel::COLUMN_MODEL_POSITION_X,
                                  mLengthDelegateX);
  mView->setItemDelegateForColumn(FootprintListModel::COLUMN_MODEL_POSITION_Y,
                                  mLengthDelegateY);
  mView->setItemDelegateForColumn(FootprintListModel::COLUMN_MODEL_POSITION_Z,
                                  mLengthDelegateZ);
  mView->setItemDelegateForColumn(FootprintListModel::COLUMN_MODEL_ROTATION_X,
                                  new AngleDelegate(this));
  mView->setItemDelegateForColumn(FootprintListModel::COLUMN_MODEL_ROTATION_Y,
                                  new AngleDelegate(this));
  mView->setItemDelegateForColumn(FootprintListModel::COLUMN_MODEL_ROTATION_Z,
                                  new AngleDelegate(this));
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_NAME, QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_MODEL_POSITION_X,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_MODEL_POSITION_Y,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_MODEL_POSITION_Z,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_MODEL_ROTATION_X,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_MODEL_ROTATION_Y,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_MODEL_ROTATION_Z,
      QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      FootprintListModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &FootprintListModel::add);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &FootprintListModel::remove);
  connect(mView.data(), &EditableTableWidget::btnCopyClicked, mModel.data(),
          &FootprintListModel::copy);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &FootprintListModel::moveUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &FootprintListModel::moveDown);
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

void FootprintListEditorWidget::setFrameStyle(int style) noexcept {
  mView->setFrameStyle(style);
}

void FootprintListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void FootprintListEditorWidget::setReferences(Package* package,
                                              UndoStack* stack) noexcept {
  mModel->setPackage(package);
  mModel->setUndoStack(stack);
}

void FootprintListEditorWidget::setLengthUnit(const LengthUnit& unit) noexcept {
  mLengthDelegateX->setUnit(unit);
  mLengthDelegateY->setUnit(unit);
  mLengthDelegateZ->setUnit(unit);
}

void FootprintListEditorWidget::setCurrentIndex(int index) noexcept {
  mView->setCurrentIndex(mModel->index(index, 0));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
