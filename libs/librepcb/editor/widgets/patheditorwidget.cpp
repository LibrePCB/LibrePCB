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
#include "patheditorwidget.h"

#include "../geometry/pathmodel.h"
#include "../model/angledelegate.h"
#include "../model/lengthdelegate.h"
#include "editabletablewidget.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PathEditorWidget::PathEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new PathModel(this)),
    mView(new EditableTableWidget(this)),
    mLengthDelegateX(new LengthDelegate(this)),
    mLengthDelegateY(new LengthDelegate(this)) {
  mView->setShowMoveButtons(true);
  mView->setShowCopyButton(true);
  mView->setModel(mModel.data());
  mView->setItemDelegateForColumn(PathModel::COLUMN_X, mLengthDelegateX);
  mView->setItemDelegateForColumn(PathModel::COLUMN_Y, mLengthDelegateY);
  mView->setItemDelegateForColumn(PathModel::COLUMN_ANGLE,
                                  new AngleDelegate(this));
  mView->horizontalHeader()->setSectionResizeMode(PathModel::COLUMN_X,
                                                  QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(PathModel::COLUMN_Y,
                                                  QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(PathModel::COLUMN_ANGLE,
                                                  QHeaderView::Stretch);
  mView->horizontalHeader()->setSectionResizeMode(
      PathModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  connect(mView.data(), &EditableTableWidget::btnAddClicked, mModel.data(),
          &PathModel::addItem);
  connect(mView.data(), &EditableTableWidget::btnCopyClicked, mModel.data(),
          &PathModel::copyItem);
  connect(mView.data(), &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &PathModel::removeItem);
  connect(mView.data(), &EditableTableWidget::btnMoveUpClicked, mModel.data(),
          &PathModel::moveItemUp);
  connect(mView.data(), &EditableTableWidget::btnMoveDownClicked, mModel.data(),
          &PathModel::moveItemDown);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());
}

PathEditorWidget::~PathEditorWidget() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PathEditorWidget::setReadOnly(bool readOnly) noexcept {
  mView->setReadOnly(readOnly);
}

void PathEditorWidget::setPath(const Path& path) noexcept {
  mModel->setPath(path);
}

const Path& PathEditorWidget::getPath() const noexcept {
  return mModel->getPath();
}

void PathEditorWidget::setLengthUnit(const LengthUnit& unit) noexcept {
  mLengthDelegateX->setUnit(unit);
  mLengthDelegateY->setUnit(unit);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
