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
#include "devicepinoutlistmodel.h"

#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmddevicepadsignalmapitemedit.h"
#include "../cmp/componentsignalnamelistmodel.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DevicePinoutListModel::DevicePinoutListModel(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mPads(nullptr),
    mSignals(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &DevicePinoutListModel::listEdited) {
}

DevicePinoutListModel::~DevicePinoutListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DevicePinoutListModel::setReferences(
    DevicePadSignalMap* list, const PackagePadList* pads,
    const std::shared_ptr<ComponentSignalNameListModel>& sigs,
    UndoStack* stack) noexcept {
  mUndoStack = stack;

  if ((list == mList) && (pads == mPads) && (sigs == mSignals)) return;

  if (mSignals) {
    disconnect(mSignals.get(), &ComponentSignalNameListModel::modified, this,
               &DevicePinoutListModel::refresh);
  }

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mPads = pads;
  mSignals = sigs;

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);
  }

  if (mSignals) {
    connect(mSignals.get(), &ComponentSignalNameListModel::modified, this,
            &DevicePinoutListModel::refresh);
  }

  refresh();
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t DevicePinoutListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::DevicePinoutData> DevicePinoutListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void DevicePinoutListModel::set_row_data(
    std::size_t i, const ui::DevicePinoutData& data) noexcept {
  if ((!mList) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  if (auto obj = mList->value(i)) {
    try {
      std::unique_ptr<CmdDevicePadSignalMapItemEdit> cmd(
          new CmdDevicePadSignalMapItemEdit(obj));
      if ((data.signal_index != mItems[i].signal_index) && mSignals) {
        cmd->setSignalUuid(mSignals->getUuid(data.signal_index));
      }
      execCmd(cmd.release());
    } catch (const Exception& e) {
      qCritical() << e.getMsg();
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::DevicePinoutData DevicePinoutListModel::createItem(
    const DevicePadSignalMapItem& obj) noexcept {
  QString name = obj.getPadUuid().toStr().left(8);
  if (mPads) {
    if (auto pad = mPads->find(obj.getPadUuid())) {
      name = *pad->getName();
    }
  }
  int sigIndex = -1;
  if (mSignals) {
    sigIndex = mSignals->getIndexOf(obj.getSignalUuid());
  }

  return ui::DevicePinoutData{
      q2s(name),  // Pad name
      obj.getSignalUuid() ? q2s(obj.getSignalUuid()->toStr())
                          : slint::SharedString(),  // Signal UUID
      sigIndex,  // Signal index
  };
}

void DevicePinoutListModel::refresh() noexcept {
  mItems.clear();
  if (mList) {
    for (const auto& obj : *mList) {
      mItems.append(createItem(obj));
    }
  }
  notify_reset();
}

void DevicePinoutListModel::listEdited(
    const DevicePadSignalMap& list, int index,
    const std::shared_ptr<const DevicePadSignalMapItem>& item,
    DevicePadSignalMap::Event event) noexcept {
  Q_UNUSED(list);

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
                    "DevicePinoutListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void DevicePinoutListModel::execCmd(UndoCommand* cmd) {
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
