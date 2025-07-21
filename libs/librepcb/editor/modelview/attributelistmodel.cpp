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

#include "../undocommand.h"
#include "../undostack.h"
#include "../utils/slinthelpers.h"
#include "cmd/cmdattributeedit.h"

#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attrtypestring.h>

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

AttributeListModel::AttributeListModel(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &AttributeListModel::listEdited) {
}

AttributeListModel::~AttributeListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void AttributeListModel::setReferences(AttributeList* list,
                                       UndoStack* stack) noexcept {
  mUndoStack = stack;

  if (list == mList) return;

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mItems.clear();

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);

    for (const auto& obj : *mList) {
      mItems.append(createItem(obj));
    }

    // Add the "New attribute..." row.
    mItems.append(createLastItem());
  }

  notify_reset();
}

void AttributeListModel::apply() {
  if ((!mList) || ((mList->count() + 1) != mItems.count())) {
    return;
  }

  for (int i = 0; i < mItems.count(); ++i) {
    auto& item = mItems[i];
    auto obj = mList->value(i);
    const QString keyStr = s2q(item.key);
    const AttributeType* type = AttributeType::getAllTypes().value(item.type);
    QString value = s2q(item.value);
    if (type && ((!obj) || (value != obj->getValue()))) {
      // Clean value & remove unit suffix.
      value = value.trimmed();
      type->tryExtractUnitFromValue(value);
    }
    const AttributeUnit* unit =
        type ? type->getAvailableUnits().value(item.unit) : nullptr;
    if (obj) {
      // Modify existing attribute.
      std::unique_ptr<CmdAttributeEdit> cmd(new CmdAttributeEdit(*obj));
      if ((keyStr != obj->getKey()) && (item.key_error.empty())) {
        cmd->setKey(validateKeyOrThrow(cleanAttributeKey(keyStr)));
      } else {
        item.key = obj ? q2s(*obj->getKey()) : slint::SharedString();
        item.key_error = slint::SharedString();
        notify_row_changed(i);
      }
      if (type && type->isValueValid(value) &&
          (unit || type->getAvailableUnits().isEmpty())) {
        cmd->setType(*type);
        cmd->setValue(value);
        cmd->setUnit(unit);
      } else {
        item.type = AttributeType::getAllTypes().indexOf(&obj->getType());
        item.value = q2s(obj->getValue());
        item.value_valid = true;
        item.unit = obj->getType().getAvailableUnits().indexOf(obj->getUnit());
        notify_row_changed(i);
      }
      execCmd(cmd.release());
    } else if ((!keyStr.trimmed().isEmpty()) && item.key_error.empty() &&
               type && (unit || type->getAvailableUnits().isEmpty())) {
      // Add new attribute.
      item = createLastItem();  // Reset row.
      notify_row_changed(i);
      auto obj = std::make_shared<Attribute>(
          validateKeyOrThrow(cleanAttributeKey(keyStr)), *type, value.trimmed(),
          unit);
      execCmd(new CmdAttributeInsert(*mList, obj, mItems.count()));
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t AttributeListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::AttributeData> AttributeListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void AttributeListModel::set_row_data(std::size_t i,
                                      const ui::AttributeData& data) noexcept {
  if ((!mList) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  auto obj = mList->value(i);
  if (data.action != ui::AttributeAction::None) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, i, obj, a = data.action]() { trigger(i, obj, a); },
        Qt::QueuedConnection);
  } else {
    const bool typeModified = data.type != mItems[i].type;
    bool valueModified = data.value != mItems[i].value;
    bool unitModified = data.unit != mItems[i].unit;

    // Get new data.
    const QString key = s2q(data.key);
    const AttributeType* type = AttributeType::getAllTypes().value(data.type);
    QString value = s2q(data.value);
    const AttributeUnit* unit =
        type ? type->getAvailableUnits().value(data.unit) : nullptr;

    // Check if the key is a duplicate.
    const bool duplicate = ((!obj) || (key != obj->getKey())) &&
        mList->find(cleanAttributeKey(key));

    // If the type was modified, reset to default unit and make sure the
    // value is valid.
    if (typeModified && type) {
      if (!type->isValueValid(value)) {
        value.clear();
        valueModified = true;
      }
      unit = type->getDefaultUnit();
      unitModified = true;
    }

    // If the value was modified, try to extract the unit from it.
    // Since the UI update doesn't work properly yet, we keep the unit in the
    // value but perform the validity check without it.
    QString valueWithoutUnit = value.trimmed();
    if (valueModified) {
      value = value.trimmed();
      if (const AttributeUnit* newUnit =
              type->tryExtractUnitFromValue(valueWithoutUnit)) {
        unit = newUnit;
        unitModified = true;
      }
    }

    // Update UI data.
    mItems[i].key = data.key;
    if ((!obj) && key.trimmed().isEmpty()) {
      mItems[i].key_error = slint::SharedString();
    } else {
      validateAttributeKey(key, mItems[i].key_error, duplicate);
    }
    if (typeModified) {
      mItems[i].type = AttributeType::getAllTypes().indexOf(type);
    }
    if (valueModified) {
      mItems[i].value = q2s(value);
      mItems[i].value_valid = type && type->isValueValid(valueWithoutUnit);
    }
    if (unitModified && type) {
      mItems[i].unit = type->getAvailableUnits().indexOf(unit);
    }
    notify_row_changed(i);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::AttributeData AttributeListModel::createItem(
    const Attribute& obj) noexcept {
  return ui::AttributeData{
      q2s(*obj.getKey()),  // Key
      slint::SharedString(),  // Key error
      static_cast<int>(
          AttributeType::getAllTypes().indexOf(&obj.getType())),  // Type
      q2s(obj.getValue()),  // Value
      obj.getType().isValueValid(obj.getValue()),  // Value valid
      static_cast<int>(
          obj.getType().getAvailableUnits().indexOf(obj.getUnit())),  // Unit
      ui::AttributeAction::None,  // Action
  };
}

ui::AttributeData AttributeListModel::createLastItem() noexcept {
  return ui::AttributeData{
      slint::SharedString(),  // Key
      slint::SharedString(),  // Key error
      static_cast<int>(AttributeType::getAllTypes().indexOf(
          &AttrTypeString::instance())),  // Type
      slint::SharedString(),  // Value
      true,  // Value valid
      -1,  // Unit
      ui::AttributeAction::None,  // Action
  };
}

void AttributeListModel::trigger(int index, std::shared_ptr<Attribute> obj,
                                 ui::AttributeAction a) noexcept {
  if ((!mList) || (!obj) || (mList->value(index) != obj)) {
    return;
  }

  try {
    if (a == ui::AttributeAction::MoveUp) {
      execCmd(new CmdAttributesSwap(*mList, index, index - 1));
    } else if (a == ui::AttributeAction::Delete) {
      execCmd(new CmdAttributeRemove(*mList, obj.get()));
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void AttributeListModel::listEdited(
    const AttributeList& list, int index,
    const std::shared_ptr<const Attribute>& item,
    AttributeList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case AttributeList::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case AttributeList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case AttributeList::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "AttributeListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void AttributeListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

AttributeKey AttributeListModel::validateKeyOrThrow(const QString& name) const {
  if (mList && mList->contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("There is already an attribute with the name \"%1\".").arg(name));
  }
  return AttributeKey(name);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
