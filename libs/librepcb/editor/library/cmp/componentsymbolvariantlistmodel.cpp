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
#include "componentsymbolvariantlistmodel.h"

#include "../../undostack.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"

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

ComponentSymbolVariantListModel::ComponentSymbolVariantListModel(
    QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mSymbolVariantList(nullptr),
    mUndoStack(nullptr),
    mNewName(),
    mNewDescription(),
    mNewNorm(),
    mOnEditedSlot(*this,
                  &ComponentSymbolVariantListModel::symbolVariantListEdited) {
}

ComponentSymbolVariantListModel::~ComponentSymbolVariantListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSymbolVariantListModel::setSymbolVariantList(
    ComponentSymbolVariantList* list) noexcept {
  emit beginResetModel();

  if (mSymbolVariantList) {
    mSymbolVariantList->onEdited.detach(mOnEditedSlot);
  }

  mSymbolVariantList = list;

  if (mSymbolVariantList) {
    mSymbolVariantList->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void ComponentSymbolVariantListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void ComponentSymbolVariantListModel::add(
    const QPersistentModelIndex& itemIndex) noexcept {
  Q_UNUSED(itemIndex);
  if (!mSymbolVariantList) {
    return;
  }

  try {
    std::shared_ptr<ComponentSymbolVariant> sv =
        std::make_shared<ComponentSymbolVariant>(Uuid::createRandom(), mNewNorm,
                                                 validateNameOrThrow(mNewName),
                                                 mNewDescription);
    execCmd(new CmdComponentSymbolVariantInsert(*mSymbolVariantList, sv));
    mNewName = QString();
    mNewDescription = QString();
    mNewNorm = QString();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantListModel::remove(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mSymbolVariantList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(itemIndex.data(Qt::EditRole).toString());
    std::shared_ptr<ComponentSymbolVariant> sv = mSymbolVariantList->get(uuid);
    execCmd(new CmdComponentSymbolVariantRemove(*mSymbolVariantList, sv.get()));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantListModel::moveUp(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mSymbolVariantList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(itemIndex.data(Qt::EditRole).toString());
    int index = mSymbolVariantList->indexOf(uuid);
    if ((index >= 1) && (index < mSymbolVariantList->count())) {
      execCmd(new CmdComponentSymbolVariantsSwap(*mSymbolVariantList, index,
                                                 index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantListModel::moveDown(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mSymbolVariantList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(itemIndex.data(Qt::EditRole).toString());
    int index = mSymbolVariantList->indexOf(uuid);
    if ((index >= 0) && (index < mSymbolVariantList->count() - 1)) {
      execCmd(new CmdComponentSymbolVariantsSwap(*mSymbolVariantList, index,
                                                 index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int ComponentSymbolVariantListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mSymbolVariantList) {
    return mSymbolVariantList->count() + 1;
  }
  return 0;
}

int ComponentSymbolVariantListModel::columnCount(
    const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant ComponentSymbolVariantListModel::data(const QModelIndex& index,
                                               int role) const {
  if (!index.isValid() || !mSymbolVariantList) {
    return QVariant();
  }

  std::shared_ptr<ComponentSymbolVariant> item =
      mSymbolVariantList->value(index.row());
  switch (index.column()) {
    case COLUMN_NAME: {
      QString name = item ? *item->getNames().getDefaultValue() : mNewName;
      bool showHint = (!item) && mNewName.isEmpty();
      QString hint = tr("Symbol variant name");
      switch (role) {
        case Qt::DisplayRole:
          if (item && (index.row() == 0) && (mSymbolVariantList->count() > 1)) {
            return name + " [" + tr("default") + "]";
          } else {
            return showHint ? hint : name;
          }
        case Qt::ToolTipRole:
          return showHint ? hint : QVariant();
        case Qt::EditRole:
          return name;
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
    case COLUMN_DESCRIPTION: {
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return item ? item->getDescriptions().getDefaultValue()
                      : mNewDescription;
        default:
          return QVariant();
      }
    }
    case COLUMN_NORM: {
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return item ? item->getNorm() : mNewNorm;
        default:
          return QVariant();
      }
    }
    case COLUMN_SYMBOLCOUNT: {
      switch (role) {
        case Qt::DisplayRole:
          return item ? item->getSymbolItems().count() : QVariant();
        case Qt::TextAlignmentRole:
          return Qt::AlignCenter;
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

QVariant ComponentSymbolVariantListModel::headerData(
    int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_NAME:
          return tr("Name");
        case COLUMN_DESCRIPTION:
          return tr("Description");
        case COLUMN_NORM:
          return tr("Norm");
        case COLUMN_SYMBOLCOUNT:
          return tr("Symbols");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mSymbolVariantList && (role == Qt::DisplayRole)) {
      std::shared_ptr<ComponentSymbolVariant> item =
          mSymbolVariantList->value(section);
      return item ? QString::number(section + 1) : tr("New:");
    } else if (mSymbolVariantList && (role == Qt::ToolTipRole)) {
      std::shared_ptr<ComponentSymbolVariant> item =
          mSymbolVariantList->value(section);
      return item ? item->getUuid().toStr() : tr("Add a new symbol variant");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

Qt::ItemFlags ComponentSymbolVariantListModel::flags(
    const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() != COLUMN_SYMBOLCOUNT) &&
      (index.column() != COLUMN_ACTIONS)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool ComponentSymbolVariantListModel::setData(const QModelIndex& index,
                                              const QVariant& value, int role) {
  if (!mSymbolVariantList) {
    return false;
  }

  try {
    std::shared_ptr<ComponentSymbolVariant> item =
        mSymbolVariantList->value(index.row());
    std::unique_ptr<CmdComponentSymbolVariantEdit> cmd;
    if (item) {
      cmd.reset(new CmdComponentSymbolVariantEdit(*item));
    }
    if ((index.column() == COLUMN_NAME) && role == Qt::EditRole) {
      QString name = value.toString().trimmed();
      QString cleanedName = cleanElementName(name);
      if (cmd) {
        LocalizedNameMap names = item->getNames();
        if (cleanedName != names.getDefaultValue()) {
          names.setDefaultValue(validateNameOrThrow(cleanedName));
          cmd->setNames(names);
        }
      } else {
        mNewName = cleanedName;
      }
    } else if ((index.column() == COLUMN_DESCRIPTION) && role == Qt::EditRole) {
      QString description = value.toString().trimmed();
      if (cmd) {
        LocalizedDescriptionMap descriptions = item->getDescriptions();
        descriptions.setDefaultValue(description);
        cmd->setDescriptions(descriptions);
      } else {
        mNewDescription = description;
      }
    } else if ((index.column() == COLUMN_NORM) && role == Qt::EditRole) {
      QString norm = value.toString().trimmed();
      if (cmd) {
        cmd->setNorm(norm);
      } else {
        mNewNorm = norm;
      }
    } else {
      return false;  // do not execute command!
    }
    if (cmd) {
      execCmd(cmd.release());
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

void ComponentSymbolVariantListModel::symbolVariantListEdited(
    const ComponentSymbolVariantList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariant>& variant,
    ComponentSymbolVariantList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(variant);
  switch (event) {
    case ComponentSymbolVariantList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case ComponentSymbolVariantList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case ComponentSymbolVariantList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning()
          << "Unhandled switch-case in "
             "ComponentSymbolVariantListModel::symbolVariantListEdited():"
          << static_cast<int>(event);
      break;
  }
}

void ComponentSymbolVariantListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

ElementName ComponentSymbolVariantListModel::validateNameOrThrow(
    const QString& name) const {
  if (mSymbolVariantList && mSymbolVariantList->contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("There is already a symbol variant with the name \"%1\".")
            .arg(name));
  }
  return ElementName(name);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
