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
#include "devicepinoutmodel.h"

#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmddevicepadsignalmapitemedit.h"

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

DevicePinoutModel::DevicePinoutModel(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mSignals(nullptr),
    mPads(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &DevicePinoutModel::listEdited) {
}

DevicePinoutModel::~DevicePinoutModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DevicePinoutModel::setList(DevicePadSignalMap* list) noexcept {
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

void DevicePinoutModel::setSignals(const ComponentSignalList* sigs) noexcept {
  mSignals = sigs;

  // TODO
  auto list = mList;
  setList(nullptr);
  setList(list);
}

void DevicePinoutModel::setPads(const PackagePadList* pads) noexcept {
  mPads = pads;

  // TODO
  auto list = mList;
  setList(nullptr);
  setList(list);
}

void DevicePinoutModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t DevicePinoutModel::row_count() const {
  return mList ? mList->count() : 0;
}

std::optional<ui::DevicePinoutData> DevicePinoutModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void DevicePinoutModel::set_row_data(
    std::size_t i, const ui::DevicePinoutData& data) noexcept {
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

ui::DevicePinoutData DevicePinoutModel::createItem(
    const DevicePadSignalMapItem& item) noexcept {
  QString name = item.getPadUuid().toStr().left(8);
  if (mPads) {
    if (auto pad = mPads->find(item.getPadUuid())) {
      name = *pad->getName();
    }
  }
  int sigIndex = -1;
  if (mSignals && item.getSignalUuid()) {
    sigIndex = mSignals->indexOf(*item.getSignalUuid());
  }

  return ui::DevicePinoutData{
      q2s(name),  // Pad name
      sigIndex,  // Signal index
  };
}

void DevicePinoutModel::listEdited(
    const DevicePadSignalMap& list, int index,
    const std::shared_ptr<const DevicePadSignalMapItem>& item,
    DevicePadSignalMap::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(item);
  switch (event) {
    case DevicePadSignalMap::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case DevicePadSignalMap::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case DevicePadSignalMap::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "DevicePinoutModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void DevicePinoutModel::execCmd(UndoCommand* cmd) {
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
