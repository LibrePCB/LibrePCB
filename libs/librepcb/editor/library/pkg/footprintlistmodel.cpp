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
#include "footprintlistmodel.h"

#include "../../undostack.h"
#include "../cmd/cmdfootprintedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintListModel::FootprintListModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mFootprintList(nullptr),
    mUndoStack(nullptr),
    mNewName(),
    mOnEditedSlot(*this, &FootprintListModel::footprintListEdited) {
}

FootprintListModel::~FootprintListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void FootprintListModel::setFootprintList(FootprintList* list) noexcept {
  emit beginResetModel();

  if (mFootprintList) {
    mFootprintList->onEdited.detach(mOnEditedSlot);
  }

  mFootprintList = list;

  if (mFootprintList) {
    mFootprintList->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void FootprintListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void FootprintListModel::addFootprint(const QVariant& editData) noexcept {
  Q_UNUSED(editData);
  if (!mFootprintList) {
    return;
  }

  try {
    std::shared_ptr<Footprint> fpt = std::make_shared<Footprint>(
        Uuid::createRandom(), validateNameOrThrow(mNewName), "");
    execCmd(new CmdFootprintInsert(*mFootprintList, fpt));
    mNewName = QString();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::copyFootprint(const QVariant& editData) noexcept {
  if (!mFootprintList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    std::shared_ptr<const Footprint> original = mFootprintList->get(uuid);
    ElementName newName("Copy of " % original->getNames().getDefaultValue());
    std::shared_ptr<Footprint> copy(
        new Footprint(Uuid::createRandom(), newName, ""));  // can throw
    copy->getDescriptions() = original->getDescriptions();
    copy->getPads() = original->getPads();
    copy->getPolygons() = original->getPolygons();
    copy->getCircles() = original->getCircles();
    copy->getStrokeTexts() = original->getStrokeTexts();
    copy->getHoles() = original->getHoles();
    execCmd(new CmdFootprintInsert(*mFootprintList, copy));
    mNewName = QString();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::removeFootprint(const QVariant& editData) noexcept {
  if (!mFootprintList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    std::shared_ptr<Footprint> fpt = mFootprintList->get(uuid);
    execCmd(new CmdFootprintRemove(*mFootprintList, fpt.get()));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::moveFootprintUp(const QVariant& editData) noexcept {
  if (!mFootprintList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    int index = mFootprintList->indexOf(uuid);
    if ((index >= 1) && (index < mFootprintList->count())) {
      execCmd(new CmdFootprintsSwap(*mFootprintList, index, index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::moveFootprintDown(const QVariant& editData) noexcept {
  if (!mFootprintList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    int index = mFootprintList->indexOf(uuid);
    if ((index >= 0) && (index < mFootprintList->count() - 1)) {
      execCmd(new CmdFootprintsSwap(*mFootprintList, index, index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int FootprintListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mFootprintList) {
    return mFootprintList->count() + 1;
  }
  return 0;
}

int FootprintListModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant FootprintListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !mFootprintList) {
    return QVariant();
  }

  std::shared_ptr<Footprint> item = mFootprintList->value(index.row());
  switch (index.column()) {
    case COLUMN_NAME: {
      QString name = item ? *item->getNames().getDefaultValue() : mNewName;
      bool showHint = (!item) && mNewName.isEmpty();
      QString hint = tr("Footprint name");
      switch (role) {
        case Qt::DisplayRole:
          return showHint ? hint : name;
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

QVariant FootprintListModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_NAME:
          return tr("Name");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mFootprintList && (role == Qt::DisplayRole)) {
      std::shared_ptr<Footprint> item = mFootprintList->value(section);
      return item ? item->getUuid().toStr().left(8) : tr("New:");
    } else if (mFootprintList && (role == Qt::ToolTipRole)) {
      std::shared_ptr<Footprint> item = mFootprintList->value(section);
      return item ? item->getUuid().toStr() : tr("Add a new footprint");
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

Qt::ItemFlags FootprintListModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() != COLUMN_ACTIONS)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool FootprintListModel::setData(const QModelIndex& index,
                                 const QVariant& value, int role) {
  if (!mFootprintList) {
    return false;
  }

  try {
    std::shared_ptr<Footprint> item = mFootprintList->value(index.row());
    QScopedPointer<CmdFootprintEdit> cmd;
    if (item) {
      cmd.reset(new CmdFootprintEdit(*item));
    }
    if ((index.column() == COLUMN_NAME) && role == Qt::EditRole) {
      QString name = value.toString().trimmed();
      QString cleanedName = cleanElementName(name);
      if (cmd) {
        if (cleanedName != item->getNames().getDefaultValue()) {
          cmd->setName(validateNameOrThrow(cleanedName));
        }
      } else {
        mNewName = cleanedName;
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

void FootprintListModel::footprintListEdited(
    const FootprintList& list, int index,
    const std::shared_ptr<const Footprint>& footprint,
    FootprintList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(footprint);
  switch (event) {
    case FootprintList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case FootprintList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case FootprintList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "FootprintListModel::footprintListEdited()";
      break;
  }
}

void FootprintListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

ElementName FootprintListModel::validateNameOrThrow(const QString& name) const {
  if (mFootprintList) {
    for (const Footprint& footprint : *mFootprintList) {
      if (footprint.getNames().getDefaultValue() == name) {
        throw RuntimeError(
            __FILE__, __LINE__,
            tr("There is already a footprint with the name \"%1\".").arg(name));
      }
    }
  }
  return ElementName(name);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
