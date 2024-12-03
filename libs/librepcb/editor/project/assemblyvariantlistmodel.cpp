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
#include "assemblyvariantlistmodel.h"

#include "../undostack.h"
#include "cmd/cmdassemblyvariantadd.h"
#include "cmd/cmdassemblyvariantedit.h"
#include "cmd/cmdassemblyvariantremove.h"

#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>

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

AssemblyVariantListModel::AssemblyVariantListModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mParentWidget(nullptr),
    mCircuit(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &AssemblyVariantListModel::listEdited) {
}

AssemblyVariantListModel::~AssemblyVariantListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AssemblyVariantListModel::setCircuit(Circuit* circuit) noexcept {
  emit beginResetModel();

  if (mCircuit) {
    mCircuit->getAssemblyVariants().onEdited.detach(mOnEditedSlot);
  }

  mCircuit = circuit;

  if (mCircuit) {
    mCircuit->getAssemblyVariants().onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void AssemblyVariantListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

void AssemblyVariantListModel::setParentWidget(QWidget* widget) noexcept {
  mParentWidget = widget;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void AssemblyVariantListModel::copy(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mCircuit) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if (std::shared_ptr<AssemblyVariant> obj =
            mCircuit->getAssemblyVariants().value(index)) {
      int number = 2;
      QString name;
      do {
        name = *obj->getName() % "-" % QString::number(number);
        ++number;
      } while (mCircuit->getAssemblyVariants().find(name));
      std::shared_ptr<AssemblyVariant> copy = std::make_shared<AssemblyVariant>(
          Uuid::createRandom(), FileProofName(name), obj->getDescription());
      execCmd(new CmdAssemblyVariantAdd(*mCircuit, copy, obj));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void AssemblyVariantListModel::remove(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mCircuit) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if (std::shared_ptr<AssemblyVariant> obj =
            mCircuit->getAssemblyVariants().value(index)) {
      execCmd(new CmdAssemblyVariantRemove(*mCircuit, obj));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void AssemblyVariantListModel::moveUp(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mCircuit) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if ((index >= 1) && (index < mCircuit->getAssemblyVariants().count())) {
      execCmd(new CmdAssemblyVariantsSwap(mCircuit->getAssemblyVariants(),
                                          index, index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void AssemblyVariantListModel::moveDown(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mCircuit) {
    return;
  }

  try {
    const int index = itemIndex.row();
    if ((index >= 0) && (index < mCircuit->getAssemblyVariants().count() - 1)) {
      execCmd(new CmdAssemblyVariantsSwap(mCircuit->getAssemblyVariants(),
                                          index, index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int AssemblyVariantListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mCircuit) {
    return mCircuit->getAssemblyVariants().count();
  }
  return 0;
}

int AssemblyVariantListModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant AssemblyVariantListModel::data(const QModelIndex& index,
                                        int role) const {
  if (!index.isValid() || (!mCircuit)) {
    return QVariant();
  }

  std::shared_ptr<AssemblyVariant> item =
      mCircuit->getAssemblyVariants().value(index.row());
  switch (index.column()) {
    case COLUMN_NAME: {
      const QString name = item ? *item->getName() : QString();
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return name;
        default:
          return QVariant();
      }
    }
    case COLUMN_DESCRIPTION: {
      const QString desc = item ? item->getDescription() : QString();
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return desc;
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant AssemblyVariantListModel::headerData(int section,
                                              Qt::Orientation orientation,
                                              int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_NAME:
          return QString("Name");
        case COLUMN_DESCRIPTION:
          return QString("Description (optional)");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mCircuit && (role == Qt::DisplayRole)) {
      std::shared_ptr<AssemblyVariant> item =
          mCircuit->getAssemblyVariants().value(section);
      return item ? QString::number(section + 1) : tr("New:");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

Qt::ItemFlags AssemblyVariantListModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() != COLUMN_ACTIONS)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool AssemblyVariantListModel::setData(const QModelIndex& index,
                                       const QVariant& value, int role) {
  if (!mCircuit) {
    return false;
  }

  try {
    std::shared_ptr<AssemblyVariant> item =
        mCircuit->getAssemblyVariants().value(index.row());
    std::unique_ptr<CmdAssemblyVariantEdit> cmd;
    if (item) {
      cmd.reset(new CmdAssemblyVariantEdit(*mCircuit, item));
    }
    const bool editRole = (role == Qt::EditRole);
    if ((index.column() == COLUMN_NAME) && editRole) {
      const QString cleaned = cleanFileProofName(value.toString());
      if (cmd) {
        cmd->setName(FileProofName(cleaned));  // can throw
      }
    } else if ((index.column() == COLUMN_DESCRIPTION) && editRole) {
      const QString cleaned = value.toString().trimmed();
      if (cmd) {
        cmd->setDescription(cleaned);
      }
    } else {
      return false;  // do not execute command!
    }
    if (cmd) {
      execCmd(cmd.release());
    } else if (!item) {
      emit dataChanged(index, index);
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

void AssemblyVariantListModel::listEdited(
    const AssemblyVariantList& list, int index,
    const std::shared_ptr<const AssemblyVariant>& obj,
    AssemblyVariantList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(obj);
  switch (event) {
    case AssemblyVariantList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case AssemblyVariantList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case AssemblyVariantList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "AssemblyVariantListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void AssemblyVariantListModel::execCmd(UndoCommand* cmd) {
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
