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

#include <librepcb/core/library/pkg/package.h>

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

FootprintListModel::FootprintListModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mPackage(nullptr),
    mUndoStack(nullptr),
    mNewName(),
    mOnEditedSlot(*this, &FootprintListModel::footprintListEdited) {
}

FootprintListModel::~FootprintListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void FootprintListModel::setPackage(Package* package) noexcept {
  emit beginResetModel();

  if (mPackage) {
    mPackage->getFootprints().onEdited.detach(mOnEditedSlot);
  }

  mPackage = package;

  if (mPackage) {
    mPackage->getFootprints().onEdited.attach(mOnEditedSlot);
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
  if (!mPackage) {
    return;
  }

  try {
    std::shared_ptr<Footprint> fpt = std::make_shared<Footprint>(
        Uuid::createRandom(), validateNameOrThrow(mNewName), "");
    fpt->setModels(mPackage->getModels().getUuidSet());
    execCmd(new CmdFootprintInsert(mPackage->getFootprints(), fpt));
    mNewName = QString();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::copyFootprint(const QVariant& editData) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    std::shared_ptr<const Footprint> original =
        mPackage->getFootprints().get(uuid);
    ElementName newName("Copy of " % original->getNames().getDefaultValue());
    std::shared_ptr<Footprint> copy(
        new Footprint(Uuid::createRandom(), newName, ""));  // can throw
    copy->getDescriptions() = original->getDescriptions();
    copy->setModelPosition(original->getModelPosition());
    copy->setModelRotation(original->getModelRotation());
    copy->setModels(original->getModels());
    copy->getPads() = original->getPads();
    copy->getPolygons() = original->getPolygons();
    copy->getCircles() = original->getCircles();
    copy->getStrokeTexts() = original->getStrokeTexts();
    copy->getHoles() = original->getHoles();
    execCmd(new CmdFootprintInsert(mPackage->getFootprints(), copy));
    mNewName = QString();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::removeFootprint(const QVariant& editData) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    std::shared_ptr<Footprint> fpt = mPackage->getFootprints().get(uuid);
    execCmd(new CmdFootprintRemove(mPackage->getFootprints(), fpt.get()));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::moveFootprintUp(const QVariant& editData) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    int index = mPackage->getFootprints().indexOf(uuid);
    if ((index >= 1) && (index < mPackage->getFootprints().count())) {
      execCmd(
          new CmdFootprintsSwap(mPackage->getFootprints(), index, index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void FootprintListModel::moveFootprintDown(const QVariant& editData) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(editData.toString());
    int index = mPackage->getFootprints().indexOf(uuid);
    if ((index >= 0) && (index < mPackage->getFootprints().count() - 1)) {
      execCmd(
          new CmdFootprintsSwap(mPackage->getFootprints(), index, index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int FootprintListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mPackage) {
    return mPackage->getFootprints().count() + 1;
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
  if (!index.isValid() || !mPackage) {
    return QVariant();
  }

  std::shared_ptr<Footprint> item =
      mPackage->getFootprints().value(index.row());
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
    case COLUMN_MODEL_POSITION_X: {
      const Length val =
          std::get<0>(item ? item->getModelPosition() : mNewModelPosition);
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_MODEL_POSITION_Y: {
      const Length val =
          std::get<1>(item ? item->getModelPosition() : mNewModelPosition);
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_MODEL_POSITION_Z: {
      const Length val =
          std::get<2>(item ? item->getModelPosition() : mNewModelPosition);
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_MODEL_ROTATION_X: {
      const Angle val =
          std::get<0>(item ? item->getModelRotation() : mNewModelRotation);
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_MODEL_ROTATION_Y: {
      const Angle val =
          std::get<1>(item ? item->getModelRotation() : mNewModelRotation);
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
        default:
          return QVariant();
      }
    }
    case COLUMN_MODEL_ROTATION_Z: {
      const Angle val =
          std::get<2>(item ? item->getModelRotation() : mNewModelRotation);
      switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant::fromValue(val);
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
          return tr("Footprint Variants");
        case COLUMN_MODEL_POSITION_X:
          return QString("3D ΔX");
        case COLUMN_MODEL_POSITION_Y:
          return QString("3D ΔY");
        case COLUMN_MODEL_POSITION_Z:
          return QString("3D ΔZ");
        case COLUMN_MODEL_ROTATION_X:
          return QString("3D ∠X");
        case COLUMN_MODEL_ROTATION_Y:
          return QString("3D ∠Y");
        case COLUMN_MODEL_ROTATION_Z:
          return QString("3D ∠Z");
        default:
          return QVariant();
      }
    } else if ((role == Qt::TextAlignmentRole) && (section == COLUMN_NAME)) {
      return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::FontRole) {
      QFont f = QAbstractItemModel::headerData(section, orientation, role)
                    .value<QFont>();
      f.setBold(section == COLUMN_NAME);
      return f;
    }
  } else if (orientation == Qt::Vertical) {
    if (mPackage && (role == Qt::DisplayRole)) {
      std::shared_ptr<Footprint> item =
          mPackage->getFootprints().value(section);
      return item ? QString::number(section + 1) : tr("New:");
    } else if (mPackage && (role == Qt::ToolTipRole)) {
      std::shared_ptr<Footprint> item =
          mPackage->getFootprints().value(section);
      return item ? item->getUuid().toStr() : tr("Add a new footprint");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
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
  if (!mPackage) {
    return false;
  }

  try {
    std::shared_ptr<Footprint> item =
        mPackage->getFootprints().value(index.row());
    QScopedPointer<CmdFootprintEdit> cmd;
    if (item) {
      cmd.reset(new CmdFootprintEdit(*item));
    }
    const bool editRole = (role == Qt::EditRole);
    Point3D modelPos = item ? item->getModelPosition() : mNewModelPosition;
    Angle3D modelRot = item ? item->getModelRotation() : mNewModelRotation;
    if ((index.column() == COLUMN_NAME) && editRole) {
      QString name = value.toString().trimmed();
      QString cleanedName = cleanElementName(name);
      if (cmd) {
        if (cleanedName != item->getNames().getDefaultValue()) {
          cmd->setName(validateNameOrThrow(cleanedName));
        }
      } else {
        mNewName = cleanedName;
      }
    } else if ((index.column() == COLUMN_MODEL_POSITION_X) && editRole) {
      std::get<0>(modelPos) = value.value<Length>();
    } else if ((index.column() == COLUMN_MODEL_POSITION_Y) && editRole) {
      std::get<1>(modelPos) = value.value<Length>();
    } else if ((index.column() == COLUMN_MODEL_POSITION_Z) && editRole) {
      std::get<2>(modelPos) = value.value<Length>();
    } else if ((index.column() == COLUMN_MODEL_ROTATION_X) && editRole) {
      std::get<0>(modelRot) = value.value<Angle>();
    } else if ((index.column() == COLUMN_MODEL_ROTATION_Y) && editRole) {
      std::get<1>(modelRot) = value.value<Angle>();
    } else if ((index.column() == COLUMN_MODEL_ROTATION_Z) && editRole) {
      std::get<2>(modelRot) = value.value<Angle>();
    } else {
      return false;  // do not execute command!
    }
    if (cmd) {
      cmd->setModelPosition(modelPos);
      cmd->setModelRotation(modelRot);
      execCmd(cmd.take());
    } else if (!item) {
      mNewModelPosition = modelPos;
      mNewModelRotation = modelRot;
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
                    "FootprintListModel::footprintListEdited():"
                 << static_cast<int>(event);
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
  if (mPackage) {
    for (const Footprint& footprint : mPackage->getFootprints()) {
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
