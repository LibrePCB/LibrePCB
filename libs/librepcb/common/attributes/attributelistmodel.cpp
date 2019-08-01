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
#include "attributelistmodel.h"

#include "attributeunit.h"
#include "attrtypestring.h"
#include "cmd/cmdattributeedit.h"

#include <librepcb/common/undostack.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AttributeListModel::AttributeListModel(QObject* parent) noexcept
  : QAbstractTableModel(parent),
    mAttributeList(nullptr),
    mUndoStack(nullptr),
    mTypeComboBoxItems(),
    mNewKey(),
    mNewType(&AttrTypeString::instance()),
    mNewValue(),
    mNewUnit(mNewType->getDefaultUnit()),
    mOnEditedSlot(*this, &AttributeListModel::attributeListEdited) {
  foreach (const AttributeType* type, AttributeType::getAllTypes()) {
    mTypeComboBoxItems.append(
        qMakePair(type->getNameTr(), QVariant(type->getName())));
  }
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  std::sort(mTypeComboBoxItems.begin(), mTypeComboBoxItems.end(),
            [&collator](const QPair<QString, QVariant>& lhs,
                        const QPair<QString, QVariant>& rhs) {
              return collator(lhs.first, rhs.first);
            });
}

AttributeListModel::~AttributeListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AttributeListModel::setAttributeList(AttributeList* list) noexcept {
  emit beginResetModel();

  if (mAttributeList) {
    mAttributeList->onEdited.detach(mOnEditedSlot);
  }

  mAttributeList = list;

  if (mAttributeList) {
    mAttributeList->onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void AttributeListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void AttributeListModel::addAttribute(const QVariant& editData) noexcept {
  Q_UNUSED(editData);
  if (!mAttributeList) {
    return;
  }

  try {
    std::shared_ptr<Attribute> attr = std::make_shared<Attribute>(
        validateKeyOrThrow(mNewKey), *mNewType, mNewValue, mNewUnit);
    execCmd(new CmdAttributeInsert(*mAttributeList, attr));
    mNewKey   = QString();
    mNewType  = &AttrTypeString::instance();
    mNewValue = QString();
    mNewUnit  = mNewType->getDefaultUnit();
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void AttributeListModel::removeAttribute(const QVariant& editData) noexcept {
  if (!mAttributeList) {
    return;
  }

  try {
    QString                    key  = editData.toString();
    std::shared_ptr<Attribute> attr = mAttributeList->get(key);
    execCmd(new CmdAttributeRemove(*mAttributeList, attr.get()));
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void AttributeListModel::moveAttributeUp(const QVariant& editData) noexcept {
  if (!mAttributeList) {
    return;
  }

  try {
    QString key   = editData.toString();
    int     index = mAttributeList->indexOf(key);
    if ((index >= 1) && (index < mAttributeList->count())) {
      execCmd(new CmdAttributesSwap(*mAttributeList, index, index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void AttributeListModel::moveAttributeDown(const QVariant& editData) noexcept {
  if (!mAttributeList) {
    return;
  }

  try {
    QString key   = editData.toString();
    int     index = mAttributeList->indexOf(key);
    if ((index >= 0) && (index < mAttributeList->count() - 1)) {
      execCmd(new CmdAttributesSwap(*mAttributeList, index, index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int AttributeListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mAttributeList) {
    return mAttributeList->count() + 1;
  }
  return 0;
}

int AttributeListModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant AttributeListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !mAttributeList) {
    return QVariant();
  }

  std::shared_ptr<Attribute> item = mAttributeList->value(index.row());
  switch (index.column()) {
    case COLUMN_KEY: {
      QString key      = item ? *item->getKey() : mNewKey;
      bool    showHint = (!item) && mNewKey.isEmpty();
      QString hint     = tr("Attribute key");
      switch (role) {
        case Qt::DisplayRole:
          return showHint ? hint : key;
        case Qt::ToolTipRole:
          return showHint ? hint : QVariant();
        case Qt::EditRole:
          return key;
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
    case COLUMN_TYPE: {
      const AttributeType* type = item ? &item->getType() : mNewType;
      Q_ASSERT(type);
      switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
          return type->getNameTr();
        case Qt::EditRole:
          return type->getName();
        case Qt::UserRole:
          return QVariant::fromValue(mTypeComboBoxItems);
        default:
          return QVariant();
      }
    }
    case COLUMN_VALUE: {
      QString value = item ? item->getValue() : mNewValue;
      switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
          return value;
        default:
          return QVariant();
      }
    }
    case COLUMN_UNIT: {
      const AttributeType* type = item ? &item->getType() : mNewType;
      const AttributeUnit* unit = item ? item->getUnit() : mNewUnit;
      switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
          return unit ? unit->getSymbolTr() : QString();
        case Qt::EditRole:
          return unit ? unit->getName() : QVariant();  // NULL means "no unit"
        case Qt::UserRole:
          return QVariant::fromValue(buildUnitComboBoxData(*type));
        default:
          return QVariant();
      }
    }
    case COLUMN_ACTIONS: {
      switch (role) {
        case Qt::EditRole:
          return item ? *item->getKey() : QVariant();
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant AttributeListModel::headerData(int             section,
                                        Qt::Orientation orientation,
                                        int             role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_KEY:
          return tr("Key");
        case COLUMN_TYPE:
          return tr("Type");
        case COLUMN_VALUE:
          return tr("Value");
        case COLUMN_UNIT:
          return tr("Unit");
        default:
          return QVariant();
      }
    }
  } else if (orientation == Qt::Vertical) {
    if (mAttributeList && (role == Qt::DisplayRole)) {
      std::shared_ptr<Attribute> item = mAttributeList->value(section);
      return item ? QString::number(section + 1) : tr("New:");
    } else if (mAttributeList && (role == Qt::ToolTipRole)) {
      std::shared_ptr<Attribute> item = mAttributeList->value(section);
      return item ? QVariant() : tr("Add a new attribute");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    } else if (role == Qt::FontRole) {
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

Qt::ItemFlags AttributeListModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid() && (index.column() != COLUMN_ACTIONS)) {
    if (index.column() == COLUMN_UNIT) {
      std::shared_ptr<Attribute> item = mAttributeList->value(index.row());
      const AttributeType*       type = item ? &item->getType() : mNewType;
      if (type->getAvailableUnits().count() > 1) {
        f |= Qt::ItemIsEditable;
      }
    } else {
      f |= Qt::ItemIsEditable;
    }
  }
  return f;
}

bool AttributeListModel::setData(const QModelIndex& index,
                                 const QVariant& value, int role) {
  if (!mAttributeList) {
    return false;
  }

  try {
    std::shared_ptr<Attribute>       item = mAttributeList->value(index.row());
    QScopedPointer<CmdAttributeEdit> cmd;
    if (item) {
      cmd.reset(new CmdAttributeEdit(*item));
    }
    if ((index.column() == COLUMN_KEY) && role == Qt::EditRole) {
      QString key = cleanAttributeKey(value.toString().trimmed());
      if (cmd) {
        if (key != item->getKey()) {
          cmd->setKey(validateKeyOrThrow(key));
        }
      } else {
        mNewKey = key;
      }
    } else if ((index.column() == COLUMN_TYPE) && role == Qt::EditRole) {
      const AttributeType* type = &AttributeType::fromString(value.toString());
      // reset value if it is no longer valid
      QString value = item ? item->getValue() : mNewValue;
      if (!type->isValueValid(value)) value = QString();
      // reset unit if it is no longer valid
      const AttributeUnit* unit = item ? item->getUnit() : mNewUnit;
      if (!type->isUnitAvailable(unit)) unit = type->getDefaultUnit();
      if (cmd) {
        cmd->setType(*type);
        cmd->setValue(value);
        cmd->setUnit(unit);
      } else {
        mNewType  = type;
        mNewValue = value;
        mNewUnit  = unit;
      }
    } else if ((index.column() == COLUMN_VALUE) && role == Qt::EditRole) {
      QString              attrValue = value.toString().trimmed();
      const AttributeType* type      = item ? &item->getType() : mNewType;
      const AttributeUnit* unit      = type->tryExtractUnitFromValue(attrValue);
      if (cmd) {
        cmd->setValue(attrValue);
        if (unit) {
          cmd->setUnit(unit);
        }
      } else {
        mNewValue = attrValue;
        if (unit) {
          mNewUnit = unit;
        }
      }
    } else if ((index.column() == COLUMN_UNIT) && role == Qt::EditRole) {
      const AttributeType* type = item ? &item->getType() : mNewType;
      const AttributeUnit* unit = type->getUnitFromString(value.toString());
      if (cmd) {
        cmd->setUnit(unit);
      } else {
        mNewUnit = unit;
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

void AttributeListModel::attributeListEdited(
    const AttributeList& list, int index,
    const std::shared_ptr<const Attribute>& attribute,
    AttributeList::Event                    event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(attribute);
  switch (event) {
    case AttributeList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case AttributeList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case AttributeList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "AttributeListModel::attributeListEdited()";
      break;
  }
}

void AttributeListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

AttributeKey AttributeListModel::validateKeyOrThrow(const QString& key) const {
  if (mAttributeList && mAttributeList->contains(key)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("There is already an attribute with the key \"%1\"."))
            .arg(key));
  }
  return AttributeKey(key);  // can throw
}

QVector<QPair<QString, QVariant>> AttributeListModel::buildUnitComboBoxData(
    const AttributeType& type) noexcept {
  QVector<QPair<QString, QVariant>> items;
  foreach (const AttributeUnit* unit, type.getAvailableUnits()) {
    items.append(qMakePair(unit->getSymbolTr(), QVariant(unit->getName())));
  }
  return items;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
