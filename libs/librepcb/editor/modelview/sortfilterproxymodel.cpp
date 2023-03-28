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
#include "sortfilterproxymodel.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SortFilterProxyModel::SortFilterProxyModel(QObject* parent) noexcept
  : QSortFilterProxyModel(parent),
    mCollator(),
    mKeepHeaderColumnUnsorted(false),
    mKeepLastRowAtBottom(false) {
  mCollator.setCaseSensitivity(Qt::CaseInsensitive);
  mCollator.setIgnorePunctuation(false);
  mCollator.setNumericMode(true);
}

SortFilterProxyModel::~SortFilterProxyModel() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

QVariant SortFilterProxyModel::headerData(int section,
                                          Qt::Orientation orientation,
                                          int role) const {
  QAbstractItemModel* model = sourceModel();
  if (mKeepHeaderColumnUnsorted && model && (orientation == Qt::Vertical)) {
    return model->headerData(section, orientation, role);
  }
  return QSortFilterProxyModel::headerData(section, orientation, role);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

bool SortFilterProxyModel::lessThan(const QModelIndex& source_left,
                                    const QModelIndex& source_right) const {
  QAbstractItemModel* model = sourceModel();
  if (mKeepLastRowAtBottom && model) {
    if (source_left.row() == model->rowCount() - 1) {
      return sortOrder() == Qt::DescendingOrder;
    } else if (source_right.row() == model->rowCount() - 1) {
      return sortOrder() == Qt::AscendingOrder;
    }
  }

  return mCollator(source_left.data().toString(),
                   source_right.data().toString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
