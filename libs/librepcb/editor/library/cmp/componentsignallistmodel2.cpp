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
#include "componentsignallistmodel2.h"
#include "../cmd/cmdcomponentsignaledit.h"
#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentSignalListModel2::ComponentSignalListModel2(QObject* parent) noexcept
  : QObject(parent),
    mSignalList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &ComponentSignalListModel2::signalListEdited) {
}

ComponentSignalListModel2::~ComponentSignalListModel2() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSignalListModel2::setSignalList(
    ComponentSignalList* list) noexcept {
  if (list == mSignalList) return;

  if (mSignalList) {
    mSignalList->onEdited.detach(mOnEditedSlot);
  }

  mSignalList = list;
  mItems.clear();

  if (mSignalList) {
    mSignalList->onEdited.attach(mOnEditedSlot);

    for (auto sig : *mSignalList) {
      mItems.append(createItem(sig));
    }
  }

  notify_reset();
}

void ComponentSignalListModel2::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

void ComponentSignalListModel2::apply() {
  if (!mSignalList) return;

  for (auto i = 0; i < mItems.count(); ++i) {
    auto& item = mItems[i];
    if (auto sig = mSignalList->value(i)) {
      std::unique_ptr<CmdComponentSignalEdit> cmd(new CmdComponentSignalEdit(*sig));
      if (auto name = parseCircuitIdentifier(cleanCircuitIdentifier(s2q(item.name)))) {
        cmd->setName(*name);
      } else {
        item.name = q2s(*sig->getName());
        item.name_error = slint::SharedString();
        notify_row_changed(i);
      }
      cmd->setIsRequired(item.required);
      cmd->setForcedNetName(s2q(item.forced_net_name).trimmed());
      execCmd(cmd.release());
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentSignalListModel2::row_count() const {
  return mSignalList ? mSignalList->count() : 0;
}

std::optional<ui::ComponentSignalData> ComponentSignalListModel2::row_data(
    std::size_t i) const {
  return (i < mItems.count()) ? std::make_optional(mItems.at(i)) : std::nullopt;
}

void ComponentSignalListModel2::set_row_data(
    std::size_t i, const ui::ComponentSignalData& data) noexcept {
  if (i < mItems.count()) {
    mItems[i] = data;
    validateCircuitIdentifier(s2q(data.name), mItems[i].name_error);
    notify_row_changed(i);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::ComponentSignalData ComponentSignalListModel2::createItem(const ComponentSignal& sig) noexcept {
  return ui::ComponentSignalData{
    q2s(sig.getUuid().toStr().left(8)),  // ID
    q2s(*sig.getName()),  // Name
    slint::SharedString(),  // Name error
    q2s(sig.getForcedNetName()),  // Forced net name
    sig.isRequired(),  // Required
};
}

void ComponentSignalListModel2::signalListEdited(
    const ComponentSignalList& list, int index,
    const std::shared_ptr<const ComponentSignal>& signal,
    ComponentSignalList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(signal);
  switch (event) {
    case ComponentSignalList::Event::ElementAdded:
      mItems.insert(index, createItem(*signal));
      notify_row_added(index, 1);
      break;
    case ComponentSignalList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case ComponentSignalList::Event::ElementEdited:
      mItems[index] = createItem(*signal);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentSignalListModel2::signalListEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void ComponentSignalListModel2::execCmd(UndoCommand* cmd) {
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
