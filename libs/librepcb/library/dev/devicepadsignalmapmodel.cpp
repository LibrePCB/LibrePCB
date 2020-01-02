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
#include "devicepadsignalmapmodel.h"

#include "cmd/cmddevicepadsignalmapitemedit.h"

#include <librepcb/common/undostack.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DevicePadSignalMapModel::DevicePadSignalMapModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mPadSignalMap(nullptr),
    mUndoStack(nullptr),
    mSignals(),
    mPads(),
    mComboBoxItems(),
    mOnEditedSlot(*this, &DevicePadSignalMapModel::padSignalMapEdited) {
  updateComboBoxItems();
}

DevicePadSignalMapModel::~DevicePadSignalMapModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void DevicePadSignalMapModel::setPadSignalMap(
    DevicePadSignalMap* map) noexcept {
  emit beginResetModel();

  if (mPadSignalMap) {
    mPadSignalMap->onEdited.detach(mOnEditedSlot);
  }

  mPadSignalMap = map;

  if (mPadSignalMap) {
    mPadSignalMap->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void DevicePadSignalMapModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

void DevicePadSignalMapModel::setSignalList(
    const ComponentSignalList& list) noexcept {
  mSignals = list;
  updateComboBoxItems();
  emit dataChanged(index(0, COLUMN_SIGNAL),
                   index(rowCount() - 1, COLUMN_SIGNAL));
}

void DevicePadSignalMapModel::setPadList(const PackagePadList& list) noexcept {
  mPads = list;
  emit dataChanged(index(0, COLUMN_PAD), index(rowCount() - 1, COLUMN_PAD));
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int DevicePadSignalMapModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mPadSignalMap) {
    return mPadSignalMap->count();
  }
  return 0;
}

int DevicePadSignalMapModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant DevicePadSignalMapModel::data(const QModelIndex& index,
                                       int                role) const {
  if (!index.isValid() || !mPadSignalMap) {
    return QVariant();
  }

  std::shared_ptr<DevicePadSignalMapItem> item =
      mPadSignalMap->value(index.row());
  if (!item) {
    return QVariant();
  }

  switch (index.column()) {
    case COLUMN_PAD: {
      Uuid                              uuid = item->getPadUuid();
      std::shared_ptr<const PackagePad> pad  = mPads.find(uuid);
      switch (role) {
        case Qt::DisplayRole:
          return pad ? *pad->getName() : uuid.toStr();
        case Qt::ToolTipRole:
          return uuid.toStr();
        default:
          return QVariant();
      }
    }
    case COLUMN_SIGNAL: {
      tl::optional<Uuid>                     uuid = item->getSignalUuid();
      std::shared_ptr<const ComponentSignal> sig =
          uuid ? mSignals.find(*uuid) : nullptr;
      switch (role) {
        case Qt::DisplayRole:
          return sig ? *sig->getName()
                     : (uuid ? uuid->toStr() : tr("(unconnected)"));
        case Qt::EditRole:
          return uuid ? uuid->toStr() : QVariant();  // NULL means unconnected!
        case Qt::ToolTipRole:
          return uuid ? uuid->toStr() : QVariant();
        case Qt::UserRole:
          return QVariant::fromValue(mComboBoxItems);
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant DevicePadSignalMapModel::headerData(int             section,
                                             Qt::Orientation orientation,
                                             int             role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_PAD:
          return tr("Package Pad");
        case COLUMN_SIGNAL:
          return tr("Component Signal");
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mPadSignalMap && (role == Qt::DisplayRole)) {
      std::shared_ptr<DevicePadSignalMapItem> item =
          mPadSignalMap->value(section);
      return item ? item->getPadUuid().toStr().left(8) : QVariant();
    } else if (mPadSignalMap && (role == Qt::ToolTipRole)) {
      std::shared_ptr<DevicePadSignalMapItem> item =
          mPadSignalMap->value(section);
      return item ? item->getPadUuid().toStr() : QVariant();
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    } else if (role == Qt::FontRole) {
      QFont f = QAbstractTableModel::headerData(section, orientation, role)
                    .value<QFont>();
      f.setStyleHint(QFont::Monospace);  // ensure fixed column width
      f.setFamily("Monospace");
      return f;
    }
  }
  return QVariant();
}

Qt::ItemFlags DevicePadSignalMapModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() == COLUMN_SIGNAL)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool DevicePadSignalMapModel::setData(const QModelIndex& index,
                                      const QVariant& value, int role) {
  if (!mPadSignalMap) {
    return false;
  }
  try {
    if ((index.column() == COLUMN_SIGNAL) && (role == Qt::EditRole)) {
      std::shared_ptr<DevicePadSignalMapItem> item =
          mPadSignalMap->value(index.row());
      if (!item) {
        return false;
      }
      QScopedPointer<CmdDevicePadSignalMapItemEdit> cmd(
          new CmdDevicePadSignalMapItemEdit(*item));
      cmd->setSignalUuid(Uuid::tryFromString(value.toString()));
      execCmd(cmd.take());
      return true;
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DevicePadSignalMapModel::padSignalMapEdited(
    const DevicePadSignalMap& map, int index,
    const std::shared_ptr<const DevicePadSignalMapItem>& item,
    DevicePadSignalMap::Event                            event) noexcept {
  Q_UNUSED(map);
  Q_UNUSED(item);
  switch (event) {
    case DevicePadSignalMap::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case DevicePadSignalMap::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case DevicePadSignalMap::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "DevicePadSignalMapModel::padSignalMapEdited()";
      break;
  }
}

void DevicePadSignalMapModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

void DevicePadSignalMapModel::updateComboBoxItems() noexcept {
  mComboBoxItems.clear();
  for (const ComponentSignal& sig : mSignals) {
    mComboBoxItems.append(
        ComboBoxDelegate::Item{*sig.getName(), QIcon(), sig.getUuid().toStr()});
  }
  mComboBoxItems.sort();
  mComboBoxItems.insert(
      0, ComboBoxDelegate::Item{QString("(%1)").arg(tr("unconnected")), QIcon(),
                                QVariant()});
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
