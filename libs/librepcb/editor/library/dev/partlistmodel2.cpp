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
#include "partlistmodel2.h"

#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdpartedit.h"

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

PartListModel2::PartListModel2(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &PartListModel2::listEdited) {
}

PartListModel2::~PartListModel2() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PartListModel2::setList(PartList* list) noexcept {
  if (list == mList) return;

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mItems.clear();

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);

    for (auto sig : *mList) {
      mItems.append(createItem(sig));
    }
  }

  notify_reset();
}

void PartListModel2::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t PartListModel2::row_count() const {
  return mList ? mList->count() : 0;
}

std::optional<ui::DevicePartData> PartListModel2::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void PartListModel2::set_row_data(std::size_t i,
                                  const ui::DevicePartData& data) noexcept {
  /*if (mList && (i < static_cast<std::size_t>(mItems.count()))) {
    if (auto sig = mList->value(i)) {
      if (data.delete_) {
        // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
        QMetaObject::invokeMethod(
            this,
            [this, sig]() {
              try {
                execCmd(new CmdComponentSignalRemove(*mList, sig.get()));
              } catch (const Exception& e) {
                qCritical() << e.getMsg();
              }
            },
            Qt::QueuedConnection);
      } else {
        mItems[i] = data;
        validateCircuitIdentifier(s2q(data.name), mItems[i].name_error);
        notify_row_changed(i);
      }
    }
  }*/
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::DevicePartData PartListModel2::createItem(const Part& item) noexcept {
  return ui::DevicePartData{
      q2s(*item.getMpn()),  // MPN
      q2s(*item.getManufacturer()),  // Manufacturer
      nullptr,  // Attributes
  };
}

void PartListModel2::listEdited(const PartList& list, int index,
                                const std::shared_ptr<const Part>& item,
                                PartList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(item);
  switch (event) {
    case PartList::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case PartList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case PartList::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PartListModel2::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PartListModel2::execCmd(UndoCommand* cmd) {
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
