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
#include "componentpinsignalmapmodel.h"

#include "../../undostack.h"
#include "../cmd/cmdcomponentpinsignalmapitemedit.h"
#include "../libraryelementcache.h"

#include <librepcb/core/library/sym/symbol.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentPinSignalMapModel::ComponentPinSignalMapModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mSymbolVariant(nullptr),
    mSignals(nullptr),
    mSymbolsCache(nullptr),
    mUndoStack(nullptr),
    mOnItemsEditedSlot(*this, &ComponentPinSignalMapModel::symbolItemsEdited),
    mOnSignalsEditedSlot(*this, &ComponentPinSignalMapModel::signalListEdited) {
  foreach (const CmpSigPinDisplayType& type,
           CmpSigPinDisplayType::getAllTypes()) {
    mDisplayTypeComboBoxItems.append(
        ComboBoxDelegate::Item{type.getNameTr(), QIcon(), type.toString()});
  }
}

ComponentPinSignalMapModel::~ComponentPinSignalMapModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentPinSignalMapModel::setSymbolVariant(
    ComponentSymbolVariant* variant) noexcept {
  emit beginResetModel();

  mOnItemsEditedSlot.detachAll();
  mSymbolVariant = variant;
  if (mSymbolVariant) {
    mSymbolVariant->getSymbolItems().onEdited.attach(mOnItemsEditedSlot);
  }

  emit endResetModel();
}

void ComponentPinSignalMapModel::setSymbolsCache(
    const std::shared_ptr<const LibraryElementCache>& cache) noexcept {
  mSymbolsCache = cache;
  emit dataChanged(index(0, COLUMN_SYMBOL), index(rowCount() - 1, COLUMN_PIN));
}

void ComponentPinSignalMapModel::setSignalList(
    const ComponentSignalList* list) noexcept {
  mOnSignalsEditedSlot.detachAll();
  mSignals = list;
  if (mSignals) {
    mSignals->onEdited.attach(mOnSignalsEditedSlot);
  }

  updateSignalComboBoxItems();
  emit dataChanged(index(0, COLUMN_SIGNAL), index(rowCount() - 1, COLUMN_PIN));
}

void ComponentPinSignalMapModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentPinSignalMapModel::autoAssignSignals() noexcept {
  if (!mSymbolVariant || !mSignals || !mSymbolsCache) {
    return;
  }

  try {
    for (ComponentSymbolVariantItem& item : mSymbolVariant->getSymbolItems()) {
      std::shared_ptr<const Symbol> symbol =
          mSymbolsCache->getSymbol(item.getSymbolUuid());
      if (symbol) {
        for (ComponentPinSignalMapItem& map : item.getPinSignalMap()) {
          CircuitIdentifier pinName =
              symbol->getPins().get(map.getPinUuid())->getName();
          std::shared_ptr<const ComponentSignal> signal =
              mSignals->find(*pinName);
          tl::optional<Uuid> signalUuid =
              signal ? tl::make_optional(signal->getUuid()) : tl::nullopt;
          QScopedPointer<CmdComponentPinSignalMapItemEdit> cmd(
              new CmdComponentPinSignalMapItemEdit(map));
          cmd->setSignalUuid(signalUuid);
          execCmd(cmd.take());  // can throw
        }
      }
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int ComponentPinSignalMapModel::rowCount(const QModelIndex& parent) const {
  int count = 0;
  if (!parent.isValid() && mSymbolVariant) {
    for (const auto& item : mSymbolVariant->getSymbolItems()) {
      count += item.getPinSignalMap().count();
    }
  }
  return count;
}

int ComponentPinSignalMapModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant ComponentPinSignalMapModel::data(const QModelIndex& index,
                                          int role) const {
  if (!index.isValid() || !mSymbolVariant) {
    return QVariant();
  }

  int symbolItemIndex = -1;
  std::shared_ptr<ComponentSymbolVariantItem> symbolItem;
  std::shared_ptr<ComponentPinSignalMapItem> mapItem;
  getRowItem(index.row(), symbolItemIndex, symbolItem, mapItem);
  if (!symbolItem || !mapItem) {
    return QVariant();
  }

  switch (index.column()) {
    case COLUMN_SYMBOL: {
      Uuid symbolUuid = symbolItem->getSymbolUuid();
      std::shared_ptr<const Symbol> symbol;
      if (mSymbolsCache) {
        symbol = mSymbolsCache->getSymbol(symbolUuid);
      }
      switch (role) {
        case Qt::DisplayRole:
          return symbol ? *symbol->getNames().getDefaultValue()
                        : symbolUuid.toStr();
        case Qt::ToolTipRole:
          return symbolUuid.toStr();
        default:
          return QVariant();
      }
    }
    case COLUMN_PIN: {
      Uuid symbolUuid = symbolItem->getSymbolUuid();
      std::shared_ptr<const Symbol> symbol;
      if (mSymbolsCache) {
        symbol = mSymbolsCache->getSymbol(symbolUuid);
      }
      Uuid pinUuid = mapItem->getPinUuid();
      std::shared_ptr<const SymbolPin> pin =
          symbol ? symbol->getPins().find(pinUuid) : nullptr;
      QString pinName = pin ? *pin->getName() : pinUuid.toStr();
      QString pinPath;
      if (mSymbolVariant->getSymbolItems().count() > 1) {
        pinPath += QString::number(symbolItemIndex + 1) % "::";
      }
      if (!symbolItem->getSuffix()->isEmpty()) {
        pinPath += *symbolItem->getSuffix() % "::";
      }
      pinPath += pinName;
      switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
          return pinPath;
        default:
          return QVariant();
      }
    }
    case COLUMN_SIGNAL: {
      tl::optional<Uuid> uuid = mapItem->getSignalUuid();
      std::shared_ptr<const ComponentSignal> sig =
          uuid && mSignals ? mSignals->find(*uuid) : nullptr;
      switch (role) {
        case Qt::DisplayRole:
          return sig
              ? *sig->getName()
              : (uuid ? uuid->toStr() : QString("(%1)").arg(tr("unconnected")));
        case Qt::EditRole:
        case Qt::ToolTipRole:
          return uuid ? uuid->toStr() : QVariant();  // NULL means unconnected!
        case Qt::UserRole:
          return QVariant::fromValue(mSignalComboBoxItems);
        default:
          return QVariant();
      }
    }
    case COLUMN_DISPLAY: {
      switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
          return mapItem->getDisplayType().getNameTr();
        case Qt::EditRole:
          return mapItem->getDisplayType().toString();
        case Qt::UserRole:
          return QVariant::fromValue(mDisplayTypeComboBoxItems);
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant ComponentPinSignalMapModel::headerData(int section,
                                                Qt::Orientation orientation,
                                                int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_SYMBOL:
          return tr("Symbol");
        case COLUMN_PIN:
          return tr("Pin");
        case COLUMN_SIGNAL:
          return tr("Component Signal");
        case COLUMN_DISPLAY:
          return tr("Designator in Schematics");
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (role == Qt::FontRole) {
      // Actually we don't show UUIDs in the vertical header, thus monospace
      // font is not needed. However, it seems that the table rows are less
      // high if the font is set to monospace, so the tables are more compact.
      QFont f = QAbstractTableModel::headerData(section, orientation, role)
                    .value<QFont>();
      f.setStyleHint(QFont::Monospace);
      f.setFamily("Monospace");
      return f;
    }
  }
  return QVariant();
}

Qt::ItemFlags ComponentPinSignalMapModel::flags(
    const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() &&
      ((index.column() == COLUMN_SIGNAL) ||
       (index.column() == COLUMN_DISPLAY))) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool ComponentPinSignalMapModel::setData(const QModelIndex& index,
                                         const QVariant& value, int role) {
  if (!mSymbolVariant) {
    return false;
  }
  try {
    int symbolItemIndex = -1;
    std::shared_ptr<ComponentSymbolVariantItem> symbolItem;
    std::shared_ptr<ComponentPinSignalMapItem> mapItem;
    getRowItem(index.row(), symbolItemIndex, symbolItem, mapItem);
    if (!mapItem) {
      return false;
    }

    if ((index.column() == COLUMN_SIGNAL) && (role == Qt::EditRole)) {
      QScopedPointer<CmdComponentPinSignalMapItemEdit> cmd(
          new CmdComponentPinSignalMapItemEdit(*mapItem));
      cmd->setSignalUuid(Uuid::tryFromString(value.toString()));
      execCmd(cmd.take());
      return true;
    } else if ((index.column() == COLUMN_DISPLAY) && (role == Qt::EditRole)) {
      QScopedPointer<CmdComponentPinSignalMapItemEdit> cmd(
          new CmdComponentPinSignalMapItemEdit(*mapItem));
      cmd->setDisplayType(CmpSigPinDisplayType::fromString(value.toString()));
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

void ComponentPinSignalMapModel::symbolItemsEdited(
    const ComponentSymbolVariantItemList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariantItem>& item,
    ComponentSymbolVariantItemList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(item);
  switch (event) {
    case ComponentSymbolVariantItemList::Event::ElementAdded:
    case ComponentSymbolVariantItemList::Event::ElementRemoved:
    case ComponentSymbolVariantItemList::Event::ElementEdited:
      emit beginResetModel();
      emit endResetModel();
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentPinSignalMapModel::symbolItemsEdited()";
      break;
  }
}

void ComponentPinSignalMapModel::signalListEdited(
    const ComponentSignalList& list, int index,
    const std::shared_ptr<const ComponentSignal>& signal,
    ComponentSignalList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(signal);
  switch (event) {
    case ComponentSignalList::Event::ElementAdded:
    case ComponentSignalList::Event::ElementRemoved:
    case ComponentSignalList::Event::ElementEdited:
      updateSignalComboBoxItems();
      dataChanged(this->index(0, COLUMN_SIGNAL),
                  this->index(rowCount() - 1, COLUMN_SIGNAL));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentPinSignalMapModel::signalListEdited()";
      break;
  }
}

void ComponentPinSignalMapModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

void ComponentPinSignalMapModel::updateSignalComboBoxItems() noexcept {
  mSignalComboBoxItems.clear();
  if (mSignals) {
    for (const ComponentSignal& sig : *mSignals) {
      mSignalComboBoxItems.append(ComboBoxDelegate::Item{
          *sig.getName(), QIcon(), sig.getUuid().toStr()});
    }
  }
  mSignalComboBoxItems.sort();
  mSignalComboBoxItems.insert(
      0,
      ComboBoxDelegate::Item{QString("(%1)").arg(tr("unconnected")), QIcon(),
                             QVariant()});
}

void ComponentPinSignalMapModel::getRowItem(
    int row, int& symbolItemIndex,
    std::shared_ptr<ComponentSymbolVariantItem>& symbolItem,
    std::shared_ptr<ComponentPinSignalMapItem>& mapItem) const noexcept {
  int count = 0;
  for (int i = 0; i < mSymbolVariant->getSymbolItems().count(); ++i) {
    symbolItemIndex = i;
    symbolItem = mSymbolVariant->getSymbolItems().value(i);
    if (row < (count + symbolItem->getPinSignalMap().count())) {
      mapItem = symbolItem->getPinSignalMap().value(row - count);
      break;
    } else {
      count += symbolItem->getPinSignalMap().count();
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
