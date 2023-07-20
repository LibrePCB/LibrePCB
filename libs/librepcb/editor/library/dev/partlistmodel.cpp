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
#include "partlistmodel.h"

#include "../../undostack.h"
#include "../cmd/cmdpartedit.h"

#include <QtCore>
#include <QtWidgets>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PartListModel::PartListModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mPartList(nullptr),
    mUndoStack(nullptr),
    mNewMpn(),
    mNewManufacturer(),
    mOnEditedSlot(*this, &PartListModel::partListEdited) {
}

PartListModel::~PartListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PartListModel::setInitialManufacturer(const SimpleString& value) noexcept {
  mNewManufacturer = *value;
}

void PartListModel::setPartList(PartList* list) noexcept {
  emit beginResetModel();

  if (mPartList) {
    mPartList->onEdited.detach(mOnEditedSlot);
  }

  mPartList = list;

  if (mPartList) {
    mPartList->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void PartListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void PartListModel::add(const QPersistentModelIndex& itemIndex) noexcept {
  Q_UNUSED(itemIndex);
  if (!mPartList) {
    return;
  }

  try {
    std::shared_ptr<Part> obj = std::make_shared<Part>(
        SimpleString(mNewMpn), SimpleString(mNewManufacturer),
        AttributeList());  // can throw
    execCmd(new CmdPartInsert(*mPartList, obj));
    mNewMpn.clear();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void PartListModel::copy(const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPartList) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if ((index >= 0) && (index < mPartList->count())) {
      std::shared_ptr<const Part> original = mPartList->at(index);
      std::shared_ptr<Part> copy = std::make_shared<Part>(*original);
      execCmd(new CmdPartInsert(*mPartList, copy));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void PartListModel::remove(const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPartList) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if ((index >= 0) && (index < mPartList->count())) {
      execCmd(new CmdPartRemove(*mPartList, mPartList->at(index).get()));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void PartListModel::moveUp(const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPartList) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if ((index >= 1) && (index < mPartList->count())) {
      execCmd(new CmdPartsSwap(*mPartList, index, index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void PartListModel::moveDown(const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPartList) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if ((index >= 0) && (index < mPartList->count() - 1)) {
      execCmd(new CmdPartsSwap(*mPartList, index, index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int PartListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mPartList) {
    return mPartList->count() + 1;
  }
  return 0;
}

int PartListModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant PartListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !mPartList) {
    return QVariant();
  }

  std::shared_ptr<Part> item = mPartList->value(index.row());

  if ((!item) && (mPartList->isEmpty()) && (role == Qt::BackgroundRole)) {
    return QBrush(Qt::red);
  } else if ((item) && (item->isEmpty()) && (role == Qt::BackgroundRole)) {
    return QBrush(Qt::red);
  }

  switch (index.column()) {
    case COLUMN_MPN: {
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return item ? *item->getMpn() : mNewMpn;
        case Qt::ToolTipRole:
          return item
              ? QVariant()
              : tr("Exact manufacturer part number (without placeholders)");
        case Qt::BackgroundRole:
          return (item && item->getMpn()->isEmpty())
              ? QVariant(QBrush(Qt::yellow))
              : QVariant();
        default:
          return QVariant();
      }
    }
    case COLUMN_MANUFACTURER: {
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return item ? *item->getManufacturer() : mNewManufacturer;
        case Qt::ToolTipRole:
          return item ? QVariant() : tr("Name of the manufacturer");
        case Qt::BackgroundRole:
          return (item && item->getManufacturer()->isEmpty())
              ? QVariant(QBrush(Qt::yellow))
              : QVariant();
        default:
          return QVariant();
      }
    }
    case COLUMN_ATTRIBUTES: {
      switch (role) {
        case Qt::DisplayRole:
          return item ? item->getAttributeValuesTr().join(", ") : QVariant();
        case Qt::ToolTipRole:
          return item ? item->getAttributeKeyValuesTr().join("\n") : QVariant();
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant PartListModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_MPN:
          return tr("Part Number");
        case COLUMN_MANUFACTURER:
          return tr("Manufacturer");
        case COLUMN_ATTRIBUTES:
          return tr("Attributes");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mPartList && (role == Qt::DisplayRole)) {
      std::shared_ptr<Part> item = mPartList->value(section);
      return item ? QString::number(section + 1) : tr("New:");
    } else if (mPartList && (role == Qt::ToolTipRole)) {
      std::shared_ptr<Part> item = mPartList->value(section);
      return item ? QVariant() : tr("Add a new part");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

Qt::ItemFlags PartListModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() != COLUMN_ATTRIBUTES) &&
      (index.column() != COLUMN_ACTIONS)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool PartListModel::setData(const QModelIndex& index, const QVariant& value,
                            int role) {
  if (!mPartList) {
    return false;
  }

  try {
    std::shared_ptr<Part> item = mPartList->value(index.row());
    QScopedPointer<CmdPartEdit> cmd;
    if (item) {
      cmd.reset(new CmdPartEdit(*item));
    }
    if ((index.column() == COLUMN_MPN) && role == Qt::EditRole) {
      const SimpleString cleaned = cleanSimpleString(value.toString());
      if (cmd) {
        cmd->setMpn(cleaned);
      } else {
        mNewMpn = *cleaned;
      }
    } else if ((index.column() == COLUMN_MANUFACTURER) &&
               role == Qt::EditRole) {
      const SimpleString cleaned = cleanSimpleString(value.toString());
      if (cmd) {
        cmd->setManufacturer(cleaned);
      } else {
        mNewManufacturer = *cleaned;
      }
    } else {
      return false;  // do not execute command!
    }
    if (cmd) {
      execCmd(cmd.take());
    } else if (!item) {
      emit dataChanged(index, index);
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PartListModel::partListEdited(const PartList& list, int index,
                                   const std::shared_ptr<const Part>& part,
                                   PartList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(part);
  switch (event) {
    case PartList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      if (list.count() == 1) {
        // Update color of last row.
        dataChanged(this->index(list.count() - 1, 0),
                    this->index(list.count() - 1, _COLUMN_COUNT - 1));
      }
      break;
    case PartList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      if (list.isEmpty()) {
        // Update color of last row.
        dataChanged(this->index(list.count() - 1, 0),
                    this->index(list.count() - 1, _COLUMN_COUNT - 1));
      }
      break;
    case PartList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PartListModel::partListEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PartListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
