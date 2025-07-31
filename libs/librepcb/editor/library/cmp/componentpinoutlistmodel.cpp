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
#include "componentpinoutlistmodel.h"

#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdcomponentpinsignalmapitemedit.h"
#include "componentsignalnamelistmodel.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentPinoutListModel::ComponentPinoutListModel(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mSignals(nullptr),
    mPins(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &ComponentPinoutListModel::listEdited) {
}

ComponentPinoutListModel::~ComponentPinoutListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentPinoutListModel::setReferences(
    ComponentPinSignalMap* list, const SymbolPinList* pins,
    const std::shared_ptr<ComponentSignalNameListModel>& sigs,
    UndoStack* stack) noexcept {
  mUndoStack = stack;

  if ((list == mList) && (pins == mPins) && (sigs == mSignals)) return;

  if (mSignals) {
    disconnect(mSignals.get(), &ComponentSignalNameListModel::modified, this,
               &ComponentPinoutListModel::refresh);
  }

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mPins = pins;
  mSignals = sigs;

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);
  }

  if (mSignals) {
    connect(mSignals.get(), &ComponentSignalNameListModel::modified, this,
            &ComponentPinoutListModel::refresh);
  }

  refresh();
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentPinoutListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::ComponentPinoutData> ComponentPinoutListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void ComponentPinoutListModel::set_row_data(
    std::size_t i, const ui::ComponentPinoutData& data) noexcept {
  if ((!mList) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  if (auto obj = mList->value(i)) {
    try {
      std::unique_ptr<CmdComponentPinSignalMapItemEdit> cmd(
          new CmdComponentPinSignalMapItemEdit(obj));
      if ((data.signal_index != mItems[i].signal_index) && mSignals) {
        cmd->setSignalUuid(mSignals->getUuid(data.signal_index));
      }
      if (data.display_mode != mItems[i].display_mode) {
        cmd->setDisplayType(s2l(data.display_mode));
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

ui::ComponentPinoutData ComponentPinoutListModel::createItem(
    const ComponentPinSignalMapItem& obj) noexcept {
  QString name = obj.getPinUuid().toStr().left(8);
  if (mPins) {
    if (auto pin = mPins->find(obj.getPinUuid())) {
      name = *pin->getName();
    }
  }
  int sigIndex = -1;
  if (mSignals) {
    sigIndex = mSignals->getIndexOf(obj.getSignalUuid());
  }

  return ui::ComponentPinoutData{
      q2s(name),  // Pin name
      sigIndex,  // Signal index
      l2s(obj.getDisplayType()),  // Display mode
  };
}

void ComponentPinoutListModel::refresh() noexcept {
  mItems.clear();
  if (mList) {
    for (const auto& obj : *mList) {
      mItems.append(createItem(obj));
    }
  }
  notify_reset();
}

void ComponentPinoutListModel::listEdited(
    const ComponentPinSignalMap& list, int index,
    const std::shared_ptr<const ComponentPinSignalMapItem>& item,
    ComponentPinSignalMap::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case ComponentPinSignalMap::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case ComponentPinSignalMap::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case ComponentPinSignalMap::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentPinoutListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void ComponentPinoutListModel::execCmd(UndoCommand* cmd) {
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
