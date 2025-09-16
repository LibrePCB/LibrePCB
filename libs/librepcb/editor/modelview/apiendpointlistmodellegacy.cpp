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
#include "apiendpointlistmodellegacy.h"

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

ApiEndpointListModelLegacy::ApiEndpointListModelLegacy(QObject* parent) noexcept
  : QAbstractTableModel(parent), mValues(), mNewUrl() {
}

ApiEndpointListModelLegacy::~ApiEndpointListModelLegacy() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ApiEndpointListModelLegacy::setValues(
    const QList<WorkspaceSettings::ApiEndpoint>& values) noexcept {
  emit beginResetModel();
  mValues = values;
  emit endResetModel();
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void ApiEndpointListModelLegacy::add(
    const QPersistentModelIndex& itemIndex) noexcept {
  Q_UNUSED(itemIndex);

  if (!mNewUrl.isValid()) {
    return;
  }

  for (const auto& ep : mValues) {
    if (ep.url == mNewUrl) {
      QMessageBox::critical(nullptr, tr("Error"),
                            QString("URL already exists."));
      return;
    }
  }

  const bool isOfficialApi = (mNewUrl == QUrl("https://api.librepcb.org"));

  WorkspaceSettings::ApiEndpoint ep;
  ep.url = mNewUrl;
  ep.useForLibraries = isOfficialApi;
  ep.useForPartsInfo = isOfficialApi && mValues.isEmpty();
  ep.useForOrder = isOfficialApi && mValues.isEmpty();

  beginInsertRows(QModelIndex(), mValues.count(), mValues.count());
  mValues.append(ep);
  endInsertRows();

  mNewUrl = QUrl();
  emit dataChanged(index(mValues.count(), 0),
                   index(mValues.count(), _COLUMN_COUNT - 1));
}

void ApiEndpointListModelLegacy::remove(
    const QPersistentModelIndex& itemIndex) noexcept {
  int row = itemIndex.data(Qt::EditRole).toInt();
  if ((row >= 0) && (row < mValues.count())) {
    beginRemoveRows(QModelIndex(), row, row);
    mValues.removeAt(row);
    endRemoveRows();
  }
}

void ApiEndpointListModelLegacy::moveUp(
    const QPersistentModelIndex& itemIndex) noexcept {
  int row = itemIndex.data(Qt::EditRole).toInt();
  if (row >= 1) {
    mValues.move(row, row - 1);
    emit dataChanged(index(row - 1, 0), index(row, _COLUMN_COUNT - 1));
  }
}

void ApiEndpointListModelLegacy::moveDown(
    const QPersistentModelIndex& itemIndex) noexcept {
  int row = itemIndex.data(Qt::EditRole).toInt();
  if (row < (mValues.count() - 1)) {
    mValues.move(row, row + 1);
    emit dataChanged(index(row, 0), index(row + 1, _COLUMN_COUNT - 1));
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int ApiEndpointListModelLegacy::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return mValues.count() + 1;
  }
  return 0;
}

int ApiEndpointListModelLegacy::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant ApiEndpointListModelLegacy::data(const QModelIndex& index,
                                          int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  const WorkspaceSettings::ApiEndpoint* item =
      ((index.row() >= 0) && (index.row() < mValues.count()))
      ? &mValues.at(index.row())
      : nullptr;
  switch (index.column()) {
    case COLUMN_URL: {
      const QString url = item ? item->url.toString() : mNewUrl.toString();
      const bool showPlaceholder = (!item) && mNewUrl.isEmpty();
      const QString placeholder = tr("Click here a add an URL");
      switch (role) {
        case Qt::DisplayRole:
          return showPlaceholder ? placeholder : url;
        case Qt::EditRole:
          return url;
        case Qt::ForegroundRole:
          if (showPlaceholder) {
            QColor color = qApp->palette().text().color();
            color.setAlpha(128);
            return QBrush(color);
          } else {
            return QVariant();
          }
        default:
          return QVariant();
      }
    }
    case COLUMN_LIBRARIES: {
      switch (role) {
        case Qt::CheckStateRole:
          if (item) {
            return item->useForLibraries ? Qt::Checked : Qt::Unchecked;
          } else {
            return QVariant();
          }
        default:
          return QVariant();
      }
    }
    case COLUMN_PARTS: {
      switch (role) {
        case Qt::CheckStateRole:
          if (item) {
            return item->useForPartsInfo ? Qt::Checked : Qt::Unchecked;
          } else {
            return QVariant();
          }
        default:
          return QVariant();
      }
    }
    case COLUMN_ORDER: {
      switch (role) {
        case Qt::CheckStateRole:
          if (item) {
            return item->useForOrder ? Qt::Checked : Qt::Unchecked;
          } else {
            return QVariant();
          }
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

QVariant ApiEndpointListModelLegacy::headerData(int section,
                                                Qt::Orientation orientation,
                                                int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_URL:
          return tr("URL");
        case COLUMN_LIBRARIES:
          return tr("Libraries");
        case COLUMN_PARTS:
          return tr("Parts Info");
        case COLUMN_ORDER:
          return tr("Order PCB");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (role == Qt::DisplayRole) {
      return (section < mValues.count()) ? QString::number(section + 1)
                                         : tr("New:");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

Qt::ItemFlags ApiEndpointListModelLegacy::flags(
    const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid()) {
    if (index.column() == COLUMN_URL) {
      f |= Qt::ItemIsEditable;
    } else if ((index.column() != COLUMN_ACTIONS) &&
               (index.row() < mValues.count())) {
      f |= Qt::ItemIsUserCheckable;
    }
  }
  return f;
}

bool ApiEndpointListModelLegacy::setData(const QModelIndex& itemIndex,
                                         const QVariant& value, int role) {
  WorkspaceSettings::ApiEndpoint* item =
      ((itemIndex.row() >= 0) && (itemIndex.row() < mValues.count()))
      ? &mValues[itemIndex.row()]
      : nullptr;
  if ((itemIndex.column() == COLUMN_URL) && role == Qt::EditRole) {
    QUrl url = QUrl::fromUserInput(value.toString().trimmed());
    if (!item) {
      mNewUrl = url;
    } else if (url.isValid()) {
      item->url = url;
    }
  } else if ((itemIndex.column() == COLUMN_LIBRARIES) &&
             (role == Qt::CheckStateRole) && item) {
    item->useForLibraries = value.toBool();
  } else if ((itemIndex.column() == COLUMN_PARTS) &&
             (role == Qt::CheckStateRole) && item) {
    for (WorkspaceSettings::ApiEndpoint& ep : mValues) {
      ep.useForPartsInfo = value.toBool() && (&ep == item);
    }
  } else if ((itemIndex.column() == COLUMN_ORDER) &&
             (role == Qt::CheckStateRole) && item) {
    for (WorkspaceSettings::ApiEndpoint& ep : mValues) {
      ep.useForOrder = value.toBool() && (&ep == item);
    }
  } else {
    return false;
  }
  emit dataChanged(index(0, itemIndex.column()),
                   index(mValues.count(), itemIndex.column()));
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
