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
#include "objectlistmodel.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ObjectListModel::ObjectListModel(QObject* parent) : QAbstractListModel(parent) {
}

ObjectListModel::~ObjectListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ObjectListModel::insert(int index, std::shared_ptr<QObject> obj) noexcept {
  Q_ASSERT(obj);
  Q_ASSERT(!mObjects.contains(obj));
  index = qBound(0, index, mObjects.count());
  beginInsertRows(QModelIndex(), index, index);
  mObjects.insert(index, obj);
  endInsertRows();
  emit countChanged(mObjects.count());
}

/*******************************************************************************
 *  Reimplemented Methods
 ******************************************************************************/

QHash<int, QByteArray> ObjectListModel::roleNames() const noexcept {
  return QHash<int, QByteArray>{
      {ROLE_OBJECT, "item"},
  };
}

int ObjectListModel::rowCount(const QModelIndex& parent) const noexcept {
  return mObjects.count();
}

QVariant ObjectListModel::data(const QModelIndex& index,
                               int role) const noexcept {
  auto obj = mObjects.value(index.row());
  switch (role) {
    case ROLE_OBJECT: {
      return QVariant::fromValue(obj.get());
    }
    default: {
      break;
    }
  }
  return QVariant();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
