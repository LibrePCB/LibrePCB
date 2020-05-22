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
#include "componentsignallistmodel.h"

#include <librepcb/common/toolbox.h>
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/cmp/cmd/cmdcomponentsignaledit.h>

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

ComponentSignalListModel::ComponentSignalListModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mSignalList(nullptr),
    mUndoStack(nullptr),
    mNewName(),
    mNewIsRequired(false),
    mNewForcedNetName(),
    mOnEditedSlot(*this, &ComponentSignalListModel::signalListEdited) {
}

ComponentSignalListModel::~ComponentSignalListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSignalListModel::setSignalList(
    ComponentSignalList* list) noexcept {
  emit beginResetModel();

  if (mSignalList) {
    mSignalList->onEdited.detach(mOnEditedSlot);
  }

  mSignalList = list;

  if (mSignalList) {
    mSignalList->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void ComponentSignalListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void ComponentSignalListModel::addSignal(const QVariant& editData) noexcept {
  Q_UNUSED(editData);
  if (!mSignalList) {
    return;
  }

  try {
    QScopedPointer<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add component signal(s)")));
    foreach (const QString& name, Toolbox::expandRangesInString(mNewName)) {
      std::shared_ptr<ComponentSignal> sig = std::make_shared<ComponentSignal>(
          Uuid::createRandom(), validateNameOrThrow(name),
          SignalRole::passive(), mNewForcedNetName, mNewIsRequired, false,
          false);
      cmd->appendChild(new CmdComponentSignalInsert(*mSignalList, sig));
    }
    execCmd(cmd.take());
    mNewName          = QString();
    mNewIsRequired    = false;
    mNewForcedNetName = QString();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void ComponentSignalListModel::removeSignal(const QVariant& editData) noexcept {
  if (!mSignalList) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    std::shared_ptr<ComponentSignal> sig = mSignalList->get(uuid);
    execCmd(new CmdComponentSignalRemove(*mSignalList, sig.get()));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int ComponentSignalListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mSignalList) {
    return mSignalList->count() + 1;
  }
  return 0;
}

int ComponentSignalListModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant ComponentSignalListModel::data(const QModelIndex& index,
                                        int                role) const {
  if (!index.isValid() || !mSignalList) {
    return QVariant();
  }

  std::shared_ptr<ComponentSignal> item = mSignalList->value(index.row());
  switch (index.column()) {
    case COLUMN_NAME: {
      QString name     = item ? *item->getName() : mNewName;
      bool    showHint = (!item) && mNewName.isEmpty();
      QString hint =
          tr("Signal name (may contain ranges like \"%1\")").arg("1..5");
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
    case COLUMN_ISREQUIRED: {
      bool required = item ? item->isRequired() : mNewIsRequired;
      switch (role) {
        case Qt::DisplayRole:
          return required ? tr("Required") : tr("Optional");
        case Qt::CheckStateRole:
          return required ? Qt::Checked : Qt::Unchecked;
        case Qt::ToolTipRole:
          return required ? tr("Leaving this signal unconnected in schematics "
                               "produces an ERC error.")
                          : tr("Leaving this signal unconnected in schematics "
                               "is allowed.");
        default:
          return QVariant();
      }
    }
    case COLUMN_FORCEDNETNAME: {
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return item ? item->getForcedNetName() : mNewForcedNetName;
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

QVariant ComponentSignalListModel::headerData(int             section,
                                              Qt::Orientation orientation,
                                              int             role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_NAME:
          return tr("Name");
        case COLUMN_ISREQUIRED:
          return tr("Connection");
        case COLUMN_FORCEDNETNAME:
          return tr("Forced Net");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mSignalList && (role == Qt::DisplayRole)) {
      std::shared_ptr<ComponentSignal> item = mSignalList->value(section);
      return item ? item->getUuid().toStr().left(8) : tr("New:");
    } else if (mSignalList && (role == Qt::ToolTipRole)) {
      std::shared_ptr<ComponentSignal> item = mSignalList->value(section);
      return item ? item->getUuid().toStr() : tr("Add a new signal");
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

Qt::ItemFlags ComponentSignalListModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() == COLUMN_ISREQUIRED)) {
    f |= Qt::ItemIsUserCheckable;
  } else if (index.isValid() && (index.column() != COLUMN_ACTIONS)) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool ComponentSignalListModel::setData(const QModelIndex& index,
                                       const QVariant& value, int role) {
  if (!mSignalList) {
    return false;
  }

  try {
    std::shared_ptr<ComponentSignal> item = mSignalList->value(index.row());
    QScopedPointer<CmdComponentSignalEdit> cmd;
    if (item) {
      cmd.reset(new CmdComponentSignalEdit(*item));
    }
    if ((index.column() == COLUMN_NAME) && role == Qt::EditRole) {
      QString name        = value.toString().trimmed();
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
    } else if ((index.column() == COLUMN_ISREQUIRED) &&
               role == Qt::CheckStateRole) {
      bool required = value.toInt() == Qt::Checked;
      if (cmd) {
        cmd->setIsRequired(required);
      } else {
        mNewIsRequired = required;
      }
    } else if ((index.column() == COLUMN_FORCEDNETNAME) &&
               role == Qt::EditRole) {
      QString forcedNetName = cleanForcedNetName(value.toString());
      if (cmd) {
        cmd->setForcedNetName(forcedNetName);
      } else {
        mNewForcedNetName = forcedNetName;
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

void ComponentSignalListModel::signalListEdited(
    const ComponentSignalList& list, int index,
    const std::shared_ptr<const ComponentSignal>& signal,
    ComponentSignalList::Event                    event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(signal);
  switch (event) {
    case ComponentSignalList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case ComponentSignalList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case ComponentSignalList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentSignalListModel::signalListEdited()";
      break;
  }
}

void ComponentSignalListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

CircuitIdentifier ComponentSignalListModel::validateNameOrThrow(
    const QString& name) const {
  if (mSignalList && mSignalList->contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("There is already a signal with the name \"%1\"."))
            .arg(name));
  }
  return CircuitIdentifier(name);  // can throw
}

QString ComponentSignalListModel::cleanForcedNetName(
    const QString& name) noexcept {
  // Same as cleanCircuitIdentifier(), but allowing '{' and '}' because it's
  // allowed to have attribute placeholders in a forced net name. Also remove
  // spaces because they must not be replaced by underscores inside {{ and }}.
  return Toolbox::cleanUserInputString(
      name, QRegularExpression("[^-a-zA-Z0-9_+/!?@#$\\{\\}]"), true, false,
      false, "");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
