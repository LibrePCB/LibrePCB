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
#include "packagepadlistmodel.h"

#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../cmd/cmdpackagepadedit.h"

#include <librepcb/core/utils/toolbox.h>

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

PackagePadListModel::PackagePadListModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mPadList(nullptr),
    mUndoStack(nullptr),
    mNewName(),
    mOnEditedSlot(*this, &PackagePadListModel::padListEdited) {
}

PackagePadListModel::~PackagePadListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackagePadListModel::setPadList(PackagePadList* list) noexcept {
  emit beginResetModel();

  if (mPadList) {
    mPadList->onEdited.detach(mOnEditedSlot);
  }

  mPadList = list;

  if (mPadList) {
    mPadList->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void PackagePadListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void PackagePadListModel::addPad(const QVariant& editData) noexcept {
  Q_UNUSED(editData);
  if (!mPadList) {
    return;
  }

  try {
    QScopedPointer<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add package pad(s)")));
    // if no name is set we search for the next free numerical pad name
    if (mNewName.isEmpty()) {
      mNewName = getNextPadNameProposal();
    }
    foreach (const QString& name, Toolbox::expandRangesInString(mNewName)) {
      std::shared_ptr<PackagePad> pad = std::make_shared<PackagePad>(
          Uuid::createRandom(), validateNameOrThrow(name));
      cmd->appendChild(new CmdPackagePadInsert(*mPadList, pad));
    }
    execCmd(cmd.take());
    mNewName = QString();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void PackagePadListModel::removePad(const QVariant& editData) noexcept {
  if (!mPadList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    std::shared_ptr<PackagePad> pad = mPadList->get(uuid);
    execCmd(new CmdPackagePadRemove(*mPadList, pad.get()));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int PackagePadListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mPadList) {
    return mPadList->count() + 1;
  }
  return 0;
}

int PackagePadListModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant PackagePadListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !mPadList) {
    return QVariant();
  }

  std::shared_ptr<PackagePad> item = mPadList->value(index.row());
  switch (index.column()) {
    case COLUMN_NAME: {
      QString name = item ? *item->getName() : mNewName;
      bool showHint = (!item) && mNewName.isEmpty();
      QString hint =
          tr("Pad name (may contain ranges like \"%1\")").arg("1..5");
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

QVariant PackagePadListModel::headerData(int section,
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
    if (mPadList && (role == Qt::DisplayRole)) {
      std::shared_ptr<PackagePad> item = mPadList->value(section);
      return item ? QString::number(section + 1) : tr("New:");
    } else if (mPadList && (role == Qt::ToolTipRole)) {
      std::shared_ptr<PackagePad> item = mPadList->value(section);
      return item ? item->getUuid().toStr() : tr("Add a new pad");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

Qt::ItemFlags PackagePadListModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid()) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool PackagePadListModel::setData(const QModelIndex& index,
                                  const QVariant& value, int role) {
  if (!mPadList) {
    return false;
  }

  try {
    std::shared_ptr<PackagePad> item = mPadList->value(index.row());
    QScopedPointer<CmdPackagePadEdit> cmd;
    if (item) {
      cmd.reset(new CmdPackagePadEdit(*item));
    }
    if ((index.column() == COLUMN_NAME) && role == Qt::EditRole) {
      QString name = value.toString().trimmed();
      QString cleanedName = cleanCircuitIdentifier(name);
      if (cmd) {
        if (cleanedName != item->getName()) {
          cmd->setName(validateNameOrThrow(cleanedName));
        }
      } else {
        QStringList names = Toolbox::expandRangesInString(name);
        if (names.count() == 1 && (names.first() == name)) {
          mNewName = cleanedName;  // no ranges -> clean name
        } else {
          mNewName = name;  // contains ranges -> keep them!
        }
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

void PackagePadListModel::padListEdited(
    const PackagePadList& list, int index,
    const std::shared_ptr<const PackagePad>& pad,
    PackagePadList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(pad);
  switch (event) {
    case PackagePadList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case PackagePadList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case PackagePadList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in PackagePadList::padListEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PackagePadListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

CircuitIdentifier PackagePadListModel::validateNameOrThrow(
    const QString& name) const {
  if (mPadList && mPadList->contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("There is already a pad with the name \"%1\".").arg(name));
  }
  return CircuitIdentifier(name);  // can throw
}

QString PackagePadListModel::getNextPadNameProposal() const noexcept {
  int i = 1;
  while (mPadList->contains(QString::number(i))) {
    ++i;
  }
  return QString::number(i);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
