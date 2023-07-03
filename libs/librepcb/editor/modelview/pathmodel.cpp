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
#include "pathmodel.h"

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

PathModel::PathModel(QObject* parent) noexcept
  : QAbstractTableModel(parent), mPath(), mNewVertex() {
}

PathModel::~PathModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PathModel::setPath(const Path& path) noexcept {
  if (path != mPath) {
    emit beginResetModel();
    mPath = path;
    emit endResetModel();
    emit pathChanged(mPath);
  }
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void PathModel::add(const QPersistentModelIndex& itemIndex) noexcept {
  Q_UNUSED(itemIndex);
  beginInsertRows(QModelIndex(), mPath.getVertices().count(),
                  mPath.getVertices().count());
  mPath.addVertex(mNewVertex);
  endInsertRows();
  emit pathChanged(mPath);
}

void PathModel::copy(const QPersistentModelIndex& itemIndex) noexcept {
  int index = itemIndex.data(Qt::EditRole).toInt();
  if ((index >= 0) && (index < mPath.getVertices().count())) {
    beginInsertRows(QModelIndex(), index, index);
    mPath.insertVertex(index, mPath.getVertices().value(index));
    endInsertRows();
    emit pathChanged(mPath);
  } else {
    qWarning() << "Invalid index in PathModel::copyItem():" << index;
  }
}

void PathModel::remove(const QPersistentModelIndex& itemIndex) noexcept {
  int index = itemIndex.data(Qt::EditRole).toInt();
  if ((index >= 0) && (index < mPath.getVertices().count())) {
    beginRemoveRows(QModelIndex(), index, index);
    mPath.getVertices().remove(index);
    endRemoveRows();
    emit pathChanged(mPath);
  } else {
    qWarning() << "Invalid index in PathModel::removeItem():" << index;
  }
}

void PathModel::moveUp(const QPersistentModelIndex& itemIndex) noexcept {
  int index = itemIndex.data(Qt::EditRole).toInt();
  if ((index >= 1) && (index < mPath.getVertices().count())) {
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), index - 1);
    mPath.getVertices().insert(index - 1, mPath.getVertices().takeAt(index));
    endMoveRows();
    emit pathChanged(mPath);
  }
}

void PathModel::moveDown(const QPersistentModelIndex& itemIndex) noexcept {
  int index = itemIndex.data(Qt::EditRole).toInt();
  if ((index >= 0) && (index < mPath.getVertices().count() - 1)) {
    // Note: destination index "+2" looks strange, but is correct. See Qt docs
    // for explanation.
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), index + 2);
    mPath.getVertices().insert(index + 1, mPath.getVertices().takeAt(index));
    endMoveRows();
    emit pathChanged(mPath);
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int PathModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return mPath.getVertices().count() + 1;
  }
  return 0;
}

int PathModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant PathModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  switch (index.column()) {
    case COLUMN_X: {
      Length val =
          mPath.getVertices().value(index.row(), mNewVertex).getPos().getX();
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_Y: {
      Length val =
          mPath.getVertices().value(index.row(), mNewVertex).getPos().getY();
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_ANGLE: {
      Angle val = mPath.getVertices().value(index.row(), mNewVertex).getAngle();
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_ACTIONS: {
      switch (role) {
        case Qt::EditRole:
          return index.row();
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant PathModel::headerData(int section, Qt::Orientation orientation,
                               int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_X:
          return tr("Pos. X");
        case COLUMN_Y:
          return tr("Pos. Y");
        case COLUMN_ANGLE:
          return tr("Angle");
        case COLUMN_ACTIONS:
          return tr("Actions");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (role == Qt::DisplayRole) {
      if (section < mPath.getVertices().count()) {
        return QString::number(section + 1);
      } else {
        return tr("New:");
      }
    } else if ((role == Qt::ToolTipRole) &&
               (section == mPath.getVertices().count())) {
      return tr("Add a new vertex");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

Qt::ItemFlags PathModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() != COLUMN_ACTIONS)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool PathModel::setData(const QModelIndex& index, const QVariant& value,
                        int role) {
  try {
    Vertex* vertex = nullptr;
    if ((index.row() >= 0) && (index.row() < mPath.getVertices().count())) {
      vertex = &mPath.getVertices()[index.row()];
    }
    if ((index.column() == COLUMN_X) && role == Qt::EditRole) {
      Length x = value.value<Length>();
      if (vertex) {
        Point pos = vertex->getPos();
        pos.setX(x);
        vertex->setPos(pos);
      } else {
        Point pos = mNewVertex.getPos();
        pos.setX(x);
        mNewVertex.setPos(pos);
      }
    } else if ((index.column() == COLUMN_Y) && role == Qt::EditRole) {
      Length y = value.value<Length>();
      if (vertex) {
        Point pos = vertex->getPos();
        pos.setY(y);
        vertex->setPos(pos);
      } else {
        Point pos = mNewVertex.getPos();
        pos.setY(y);
        mNewVertex.setPos(pos);
      }
    } else if ((index.column() == COLUMN_ANGLE) && role == Qt::EditRole) {
      Angle angle = value.value<Angle>();
      if (vertex) {
        vertex->setAngle(angle);
      } else {
        mNewVertex.setAngle(angle);
      }
    } else {
      return false;
    }
    emit dataChanged(index, index);
    if (vertex) {
      emit pathChanged(mPath);
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
