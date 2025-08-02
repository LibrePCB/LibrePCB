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
#include "partlistmodel.h"

#include "../../undocommand.h"
#include "../../undostack.h"
#include "../cmd/cmdpartedit.h"
#include "parteditor.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PartListModel::PartListModel(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mUndoStack(nullptr),
    mNewPart(new Part(SimpleString(QString()), SimpleString(QString()),
                      AttributeList())),
    mOnEditedSlot(*this, &PartListModel::listEdited) {
}

PartListModel::~PartListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PartListModel::setDefaultManufacturer(const SimpleString& mfr) noexcept {
  mNewPart->setManufacturer(mfr);
  if (!mItems.isEmpty()) {
    notify_row_changed(mItems.count() - 1);
  }
}

void PartListModel::setReferences(PartList* list, UndoStack* stack) noexcept {
  if ((list == mList) && (stack == mUndoStack)) return;

  mUndoStack = stack;

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mItems.clear();

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);

    for (auto obj : mList->values()) {
      mItems.append(std::make_shared<PartEditor>(obj, mUndoStack));
    }

    // Add the "New part..." row.
    mItems.append(std::make_shared<PartEditor>(mNewPart, nullptr));
  }

  notify_reset();
}

void PartListModel::apply() {
  if ((!mList) || ((mList->count() + 1) != mItems.count())) {
    return;
  }

  for (auto editor : mItems) {
    editor->apply();
  }

  if ((!mNewPart->getMpn()->isEmpty()) &&
      (!mNewPart->getManufacturer()->isEmpty())) {
    // Copy part.
    auto obj = std::make_shared<Part>(*mNewPart);

    // Reset MPN but keep the rest.
    mNewPart->setMpn(SimpleString(QString()));
    notify_row_changed(mList->count());

    // Add new part.
    execCmd(new CmdPartInsert(*mList, obj, mItems.count()));
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t PartListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::PartData> PartListModel::row_data(std::size_t i) const {
  if (auto item = mItems.value(i)) {
    return item->getUiData();
  } else {
    return std::nullopt;
  }
}

void PartListModel::set_row_data(std::size_t i,
                                 const ui::PartData& data) noexcept {
  if ((!mList) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  auto obj = mList->value(i);
  if (data.action != ui::PartAction::None) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this, [this, i, obj, a = data.action]() { trigger(i, obj, a); },
        Qt::QueuedConnection);
  } else if (auto editor = mItems.value(i)) {
    editor->setUiData(data, static_cast<int>(i) == mList->count());
    notify_row_changed(i);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PartListModel::trigger(int index, std::shared_ptr<Part> obj,
                            ui::PartAction a) noexcept {
  if ((!mList) || (!obj) || (mList->value(index) != obj)) {
    return;
  }

  try {
    if (a == ui::PartAction::MoveUp) {
      execCmd(new CmdPartsSwap(*mList, index, index - 1));
    } else if (a == ui::PartAction::Duplicate) {
      auto copy = std::make_shared<Part>(*obj);
      execCmd(new CmdPartInsert(*mList, copy, index + 1));
    } else if (a == ui::PartAction::Delete) {
      execCmd(new CmdPartRemove(*mList, obj.get()));
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void PartListModel::listEdited(const PartList& list, int index,
                               const std::shared_ptr<const Part>& item,
                               PartList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case PartList::Event::ElementAdded: {
      auto editor = std::make_shared<PartEditor>(
          std::const_pointer_cast<Part>(item), mUndoStack);
      mItems.insert(index, editor);
      notify_row_added(index, 1);
      break;
    }
    case PartList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case PartList::Event::ElementEdited:
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PartListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PartListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
