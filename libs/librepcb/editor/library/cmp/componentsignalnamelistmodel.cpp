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
#include "componentsignalnamelistmodel.h"

#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdcomponentsignaledit.h"

#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentSignalNameListModel::ComponentSignalNameListModel(
    QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &ComponentSignalNameListModel::listEdited) {
}

ComponentSignalNameListModel::~ComponentSignalNameListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSignalNameListModel::setReferences(ComponentSignalList* list,
                                                 UndoStack* stack) noexcept {
  mUndoStack = stack;

  if (list == mList) return;

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  updateItems();

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);
  }
}

std::optional<Uuid> ComponentSignalNameListModel::getUuid(
    std::size_t i) const noexcept {
  if (auto sig = mSignalsSorted.value(i)) {
    return sig->getUuid();
  } else {
    return std::nullopt;
  }
}

int ComponentSignalNameListModel::getIndexOf(
    const std::optional<Uuid>& sig) const noexcept {
  for (int i = 0; i < mSignalsSorted.count(); ++i) {
    auto ptr = mSignalsSorted.value(i);
    if (((!ptr) && (!sig)) || (ptr && sig && (ptr->getUuid() == sig))) {
      return i;
    }
  }
  return -1;
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentSignalNameListModel::row_count() const {
  return mSignalsSorted.count();
}

std::optional<slint::SharedString> ComponentSignalNameListModel::row_data(
    std::size_t i) const {
  if (i == 0) {
    return q2s("(" % tr("unconnected").toLower() % ")");
  } else if (auto sig = mSignalsSorted.value(i)) {
    return q2s(*sig->getName());
  } else {
    return std::nullopt;
  }
}

void ComponentSignalNameListModel::set_row_data(
    std::size_t i, const slint::SharedString& data) {
  try {
    auto sig = mSignalsSorted.value(i);
    const std::optional<CircuitIdentifier> name =
        parseCircuitIdentifier(cleanCircuitIdentifier(s2q(data)));
    if (mList && sig && name && (*name != sig->getName()) &&
        (!mList->contains(**name))) {
      std::unique_ptr<CmdComponentSignalEdit> cmd(
          new CmdComponentSignalEdit(*sig));
      cmd->setName(*name);
      execCmd(cmd.release());
    }
  } catch (const Exception& e) {
    qWarning() << e.getMsg();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentSignalNameListModel::updateItems() noexcept {
  mSignalsSorted.clear();

  if (mList) {
    mSignalsSorted = mList->values();
    Toolbox::sortNumeric(
        mSignalsSorted,
        [](const QCollator& collator,
           const std::shared_ptr<ComponentSignal>& lhs,
           const std::shared_ptr<ComponentSignal>& rhs) {
          return collator(
              EditorToolbox::sortableCircuitIdentifier(*lhs->getName()),
              EditorToolbox::sortableCircuitIdentifier(*rhs->getName()));
        });
    mSignalsSorted.insert(0, nullptr);
  }

  notify_reset();
  emit modified();
}

void ComponentSignalNameListModel::listEdited(
    const ComponentSignalList& list, int index,
    const std::shared_ptr<const ComponentSignal>& item,
    ComponentSignalList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(item);

  switch (event) {
    case ComponentSignalList::Event::ElementAdded:
    case ComponentSignalList::Event::ElementRemoved:
    case ComponentSignalList::Event::ElementEdited:
      // Asynchronous to avoid recursion (-> crash) in set_row_data().
      QMetaObject::invokeMethod(this,
                                &ComponentSignalNameListModel::updateItems,
                                Qt::QueuedConnection);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentSignalNameListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void ComponentSignalNameListModel::execCmd(UndoCommand* cmd) {
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
