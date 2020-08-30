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
#include "componentsymbolvariantitemlistmodel.h"

#include "../libraryelementcache.h"

#include <librepcb/common/undostack.h>
#include <librepcb/library/cmp/cmd/cmdcomponentsymbolvariantedit.h>
#include <librepcb/library/cmp/cmd/cmdcomponentsymbolvariantitemedit.h>
#include <librepcb/library/sym/symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentSymbolVariantItemListModel::ComponentSymbolVariantItemListModel(
    QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mItemList(nullptr),
    mSymbolsCache(nullptr),
    mUndoStack(nullptr),
    mNewSymbolUuid(),
    mNewSuffix(),
    mNewIsRequired(true),
    mNewPosition(0, 0),
    mNewRotation(0),
    mOnEditedSlot(*this, &ComponentSymbolVariantItemListModel::itemListEdited) {
}

ComponentSymbolVariantItemListModel::
    ~ComponentSymbolVariantItemListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSymbolVariantItemListModel::setItemList(
    ComponentSymbolVariantItemList* list) noexcept {
  emit beginResetModel();

  mOnEditedSlot.detachAll();
  mItemList = list;
  if (mItemList) {
    mItemList->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void ComponentSymbolVariantItemListModel::setSymbolsCache(
    const std::shared_ptr<const LibraryElementCache>& cache) noexcept {
  mSymbolsCache = cache;
  emit dataChanged(index(0, COLUMN_SYMBOL),
                   index(rowCount() - 1, COLUMN_SYMBOL));
}

void ComponentSymbolVariantItemListModel::setUndoStack(
    UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void ComponentSymbolVariantItemListModel::addItem(
    const QVariant& editData) noexcept {
  Q_UNUSED(editData);
  if (!mItemList || !mSymbolsCache) {
    return;
  }

  if (!mNewSymbolUuid) {
    QMessageBox::critical(0, tr("Error"), tr("Please choose a symbol."));
    return;
  }

  try {
    std::shared_ptr<const Symbol> symbol =
        mSymbolsCache->getSymbol(*mNewSymbolUuid);
    if (!symbol) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Symbol '%1' not found in workspace library!")
                             .arg(mNewSymbolUuid->toStr()));
    }
    std::shared_ptr<ComponentSymbolVariantItem> item =
        std::make_shared<ComponentSymbolVariantItem>(
            Uuid::createRandom(), *mNewSymbolUuid, mNewPosition, mNewRotation,
            mNewIsRequired, ComponentSymbolVariantItemSuffix(mNewSuffix));
    item->getPinSignalMap() =
        ComponentPinSignalMapHelpers::create(symbol->getPins().getUuidSet());
    execCmd(new CmdComponentSymbolVariantItemInsert(*mItemList, item));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantItemListModel::removeItem(
    const QVariant& editData) noexcept {
  if (!mItemList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    std::shared_ptr<ComponentSymbolVariantItem> item = mItemList->get(uuid);
    execCmd(new CmdComponentSymbolVariantItemRemove(*mItemList, item.get()));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantItemListModel::moveItemUp(
    const QVariant& editData) noexcept {
  if (!mItemList) {
    return;
  }

  try {
    Uuid uuid  = Uuid::fromString(editData.toString());
    int  index = mItemList->indexOf(uuid);
    if ((index >= 1) && (index < mItemList->count())) {
      execCmd(
          new CmdComponentSymbolVariantItemsSwap(*mItemList, index, index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantItemListModel::moveItemDown(
    const QVariant& editData) noexcept {
  if (!mItemList) {
    return;
  }

  try {
    Uuid uuid  = Uuid::fromString(editData.toString());
    int  index = mItemList->indexOf(uuid);
    if ((index >= 0) && (index < mItemList->count() - 1)) {
      execCmd(
          new CmdComponentSymbolVariantItemsSwap(*mItemList, index, index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantItemListModel::changeSymbol(
    const QVariant& editData, const Uuid& symbol) noexcept {
  if (!mItemList || !mSymbolsCache) {
    return;
  }

  try {
    std::shared_ptr<const Symbol> sym = mSymbolsCache->getSymbol(symbol);
    if (!sym) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Symbol '%1' not found in workspace library!")
                             .arg(symbol.toStr()));
    }
    tl::optional<Uuid> uuid = Uuid::tryFromString(editData.toString());
    if (uuid) {
      std::shared_ptr<ComponentSymbolVariantItem> item = mItemList->get(*uuid);
      QScopedPointer<CmdComponentSymbolVariantItemEdit> cmd(
          new CmdComponentSymbolVariantItemEdit(*item));
      cmd->setSymbolUuid(symbol);
      cmd->setPinSignalMap(
          ComponentPinSignalMapHelpers::create(sym->getPins().getUuidSet()));
      execCmd(cmd.take());
    } else {
      mNewSymbolUuid = symbol;
      emit dataChanged(index(rowCount() - 1, COLUMN_SYMBOL),
                       index(rowCount() - 1, COLUMN_SYMBOL));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int ComponentSymbolVariantItemListModel::rowCount(
    const QModelIndex& parent) const {
  if (!parent.isValid() && mItemList) {
    return mItemList->count() + 1;
  }
  return 0;
}

int ComponentSymbolVariantItemListModel::columnCount(
    const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant ComponentSymbolVariantItemListModel::data(const QModelIndex& index,
                                                   int role) const {
  if (!index.isValid() || !mItemList) {
    return QVariant();
  }

  std::shared_ptr<ComponentSymbolVariantItem> item =
      mItemList->value(index.row());
  switch (index.column()) {
    case COLUMN_NUMBER: {
      switch (role) {
        case Qt::DisplayRole:
          return index.row() + 1;
        default:
          return QVariant();
      }
    }
    case COLUMN_SYMBOL: {
      std::shared_ptr<const Symbol> symbol;
      tl::optional<Uuid> uuid = item ? item->getSymbolUuid() : mNewSymbolUuid;
      if (mSymbolsCache && uuid) {
        symbol = mSymbolsCache->getSymbol(*uuid);
      }
      QString name = symbol ? *symbol->getNames().getDefaultValue()
                            : (uuid ? uuid->toStr() : QString());
      bool    showHint = (!item) && (!mNewSymbolUuid);
      QString hint     = tr("Choose symbol...");
      switch (role) {
        case Qt::DisplayRole:
          return showHint ? hint : name;
        case Qt::EditRole:
          return item ? item->getUuid().toStr() : QVariant();  // for the view
        case Qt::ForegroundRole:
          if (showHint) {
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
    case COLUMN_SUFFIX: {
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return item ? *item->getSuffix() : mNewSuffix;
        default:
          return QVariant();
      }
    }
    case COLUMN_ISREQUIRED: {
      bool required = item ? item->isRequired() : mNewIsRequired;
      switch (role) {
        case Qt::DisplayRole:
          return required ? tr("Required") : tr("Optional");
        case Qt::CheckStateRole:
          return required ? Qt::Checked : Qt::Unchecked;
        case Qt::ToolTipRole:
          return required
                     ? tr("Placing this symbol in schematics is mandatory.")
                     : tr("Placing this symbol in schematics is optional");
        default:
          return QVariant();
      }
    }
    case COLUMN_X: {
      switch (role) {
        case Qt::DisplayRole:
          return QString("%1mm").arg(
              item ? item->getSymbolPosition().getX().toMm()
                   : mNewPosition.getX().toMm());
        case Qt::EditRole:
          return item ? item->getSymbolPosition().getX().toMm()
                      : mNewPosition.getX().toMm();
        default:
          return QVariant();
      }
    }
    case COLUMN_Y: {
      switch (role) {
        case Qt::DisplayRole:
          return QString("%1mm").arg(
              item ? item->getSymbolPosition().getY().toMm()
                   : mNewPosition.getY().toMm());
        case Qt::EditRole:
          return item ? item->getSymbolPosition().getY().toMm()
                      : mNewPosition.getY().toMm();
        default:
          return QVariant();
      }
    }
    case COLUMN_ROTATION: {
      switch (role) {
        case Qt::DisplayRole:
          return QString("%1°").arg(item ? item->getSymbolRotation().toDeg()
                                         : mNewRotation.toDeg());
        case Qt::EditRole:
          return item ? item->getSymbolRotation().toDeg()
                      : mNewRotation.toDeg();
        default:
          return QVariant();
      }
    }
    case COLUMN_ACTIONS: {
      switch (role) {
        case Qt::EditRole:
          return item ? item->getUuid().toStr() : QVariant();
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant ComponentSymbolVariantItemListModel::headerData(
    int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_NUMBER:
          return "#";
        case COLUMN_SYMBOL:
          return tr("Symbol");
        case COLUMN_SUFFIX:
          return tr("Suffix");
        case COLUMN_ISREQUIRED:
          return tr("Placement");
        case COLUMN_X:
          return tr("Position X");
        case COLUMN_Y:
          return tr("Position Y");
        case COLUMN_ROTATION:
          return tr("Rotation");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mItemList && (role == Qt::DisplayRole)) {
      std::shared_ptr<ComponentSymbolVariantItem> item =
          mItemList->value(section);
      return item ? item->getUuid().toStr().left(8) : tr("New:");
    } else if (mItemList && (role == Qt::ToolTipRole)) {
      std::shared_ptr<ComponentSymbolVariantItem> item =
          mItemList->value(section);
      return item ? item->getUuid().toStr() : tr("Add a new symbol");
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

Qt::ItemFlags ComponentSymbolVariantItemListModel::flags(
    const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() == COLUMN_ISREQUIRED)) {
    f |= Qt::ItemIsUserCheckable;
  } else if (index.isValid() && (index.column() >= COLUMN_SUFFIX) &&
             (index.column() <= COLUMN_ROTATION)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool ComponentSymbolVariantItemListModel::setData(const QModelIndex& index,
                                                  const QVariant&    value,
                                                  int                role) {
  if (!mItemList) {
    return false;
  }

  try {
    std::shared_ptr<ComponentSymbolVariantItem> item =
        mItemList->value(index.row());
    QScopedPointer<CmdComponentSymbolVariantItemEdit> cmd;
    if (item) {
      cmd.reset(new CmdComponentSymbolVariantItemEdit(*item));
    }
    if ((index.column() == COLUMN_SUFFIX) && role == Qt::EditRole) {
      QString suffix = value.toString().trimmed();
      if (cmd) {
        cmd->setSuffix(ComponentSymbolVariantItemSuffix(suffix));
      } else {
        mNewSuffix = suffix;
      }
    } else if ((index.column() == COLUMN_ISREQUIRED) &&
               role == Qt::CheckStateRole) {
      bool required = value.toInt() == Qt::Checked;
      if (cmd) {
        cmd->setIsRequired(required);
      } else {
        mNewIsRequired = required;
      }
    } else if ((index.column() == COLUMN_X) && role == Qt::EditRole) {
      Point pos = item ? item->getSymbolPosition() : mNewPosition;
      pos.setX(Length::fromMm(value.toReal()));
      if (cmd) {
        cmd->setSymbolPosition(pos);
      } else {
        mNewPosition = pos;
      }
    } else if ((index.column() == COLUMN_Y) && role == Qt::EditRole) {
      Point pos = item ? item->getSymbolPosition() : mNewPosition;
      pos.setY(Length::fromMm(value.toReal()));
      if (cmd) {
        cmd->setSymbolPosition(pos);
      } else {
        mNewPosition = pos;
      }
    } else if ((index.column() == COLUMN_ROTATION) && role == Qt::EditRole) {
      Angle rot = Angle::fromDeg(value.toReal());
      if (cmd) {
        cmd->setSymbolRotation(rot);
      } else {
        mNewRotation = rot;
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

void ComponentSymbolVariantItemListModel::itemListEdited(
    const ComponentSymbolVariantItemList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariantItem>& item,
    ComponentSymbolVariantItemList::Event                    event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(item);
  switch (event) {
    case ComponentSymbolVariantItemList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case ComponentSymbolVariantItemList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case ComponentSymbolVariantItemList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentSymbolVariantItemListModel::itemListEdited()";
      break;
  }
}

void ComponentSymbolVariantItemListModel::execCmd(UndoCommand* cmd) {
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
}  // namespace library
}  // namespace librepcb
