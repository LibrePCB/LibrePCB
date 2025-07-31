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
#include "componentsignallistmodel.h"

#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdcomponentpinsignalmapitemedit.h"
#include "../cmd/cmdcomponentsignaledit.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/utils/toolbox.h>

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

ComponentSignalListModel::ComponentSignalListModel(QObject* parent) noexcept
  : QObject(parent),
    mComponent(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &ComponentSignalListModel::listEdited) {
}

ComponentSignalListModel::~ComponentSignalListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSignalListModel::setReferences(Component* component,
                                             UndoStack* stack) noexcept {
  mUndoStack = stack;

  if (component == mComponent) return;

  if (mComponent) {
    mComponent->getSignals().onEdited.detach(mOnEditedSlot);
  }

  mComponent = component;
  mItems.clear();

  if (mComponent) {
    mComponent->getSignals().onEdited.attach(mOnEditedSlot);

    for (auto obj : mComponent->getSignals()) {
      mItems.append(createItem(obj, mItems.count()));
    }
    updateSortOrder(false);
  }

  notify_reset();
}

bool ComponentSignalListModel::add(QString names) noexcept {
  if (!mComponent) return false;

  try {
    std::unique_ptr<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add Component Signal(s)")));
    foreach (const QString& nameStr, Toolbox::expandRangesInString(names)) {
      std::shared_ptr<ComponentSignal> obj = std::make_shared<ComponentSignal>(
          Uuid::createRandom(),
          validateNameOrThrow(cleanCircuitIdentifier(nameStr)),
          SignalRole::passive(), QString(), false, false, false);
      cmd->appendChild(
          new CmdComponentSignalInsert(mComponent->getSignals(), obj));
    }
    execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}

void ComponentSignalListModel::apply() {
  if ((!mComponent) || (mComponent->getSignals().count() != mItems.count())) {
    return;
  }

  for (int i = 0; i < mComponent->getSignals().count(); ++i) {
    auto& item = mItems[i];
    if (auto obj = mComponent->getSignals().value(i)) {
      const QString nameStr = s2q(item.name);
      const QString forcedNetNameStr = s2q(item.forced_net_name);
      std::unique_ptr<CmdComponentSignalEdit> cmd(
          new CmdComponentSignalEdit(*obj));
      if ((nameStr != obj->getName()) && (item.name_error.empty())) {
        cmd->setName(validateNameOrThrow(cleanCircuitIdentifier(nameStr)));
      } else {
        item.name = q2s(*obj->getName());
        item.name_error = slint::SharedString();
        notify_row_changed(i);
      }
      cmd->setIsRequired(item.required);
      if (forcedNetNameStr != obj->getForcedNetName()) {
        cmd->setForcedNetName(cleanForcedNetName(forcedNetNameStr));
      }
      execCmd(cmd.release());
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentSignalListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::ComponentSignalData> ComponentSignalListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void ComponentSignalListModel::set_row_data(
    std::size_t i, const ui::ComponentSignalData& data) noexcept {
  if ((!mComponent) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  if (auto obj = mComponent->getSignals().value(i)) {
    if (data.delete_) {
      // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
      QMetaObject::invokeMethod(
          this,
          [this, i, obj]() {
            try {
              if (mComponent && (mComponent->getSignals().value(i) == obj)) {
                std::unique_ptr<UndoCommandGroup> cmdGroup(
                    new UndoCommandGroup(tr("Delete Component Signal")));
                for (auto& variant : mComponent->getSymbolVariants()) {
                  for (auto& gate : variant.getSymbolItems()) {
                    for (auto& pinout : gate.getPinSignalMap().values()) {
                      if (pinout->getSignalUuid() == obj->getUuid()) {
                        std::unique_ptr<CmdComponentPinSignalMapItemEdit> cmd(
                            new CmdComponentPinSignalMapItemEdit(pinout));
                        cmd->setSignalUuid(std::nullopt);
                        cmdGroup->appendChild(cmd.release());
                      }
                    }
                  }
                }
                cmdGroup->appendChild(new CmdComponentSignalRemove(
                    mComponent->getSignals(), obj.get()));
                execCmd(cmdGroup.release());
              }
            } catch (const Exception& e) {
              qCritical() << e.getMsg();
            }
          },
          Qt::QueuedConnection);
    } else {
      mItems[i] = data;
      const QString name = s2q(data.name);
      const bool duplicate =
          (name != obj->getName()) && mComponent->getSignals().find(name);
      validateCircuitIdentifier(name, mItems[i].name_error, duplicate);
      notify_row_changed(i);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::ComponentSignalData ComponentSignalListModel::createItem(
    const ComponentSignal& obj, int sortIndex) noexcept {
  return ui::ComponentSignalData{
      q2s(obj.getUuid().toStr().left(8)),  // ID
      q2s(*obj.getName()),  // Name
      slint::SharedString(),  // Name error
      q2s(obj.getForcedNetName()),  // Forced net name
      obj.isRequired(),  // Required
      sortIndex,  // Sort index
      false,  // Delete
  };
}

void ComponentSignalListModel::updateSortOrder(bool notify) noexcept {
  // Note: The sorting needs to be done only when the underlying list data
  // was modfied, not when the UI data is changed, since this would lead to
  // reordering while the user is typing, causing focus issues etc.

  if (!mComponent) return;

  auto sorted = mComponent->getSignals().values();
  Toolbox::sortNumeric(
      sorted,
      [](const QCollator& collator, const std::shared_ptr<ComponentSignal>& lhs,
         const std::shared_ptr<ComponentSignal>& rhs) {
        return collator(
            EditorToolbox::sortableCircuitIdentifier(*lhs->getName()),
            EditorToolbox::sortableCircuitIdentifier(*rhs->getName()));
      });
  for (int i = 0; i < mComponent->getSignals().count(); ++i) {
    const int sortIndex = sorted.indexOf(mComponent->getSignals().value(i));
    if (sortIndex != mItems[i].sort_index) {
      mItems[i].sort_index = sortIndex;
      if (notify) {
        notify_row_changed(i);
      }
    }
  }
}

void ComponentSignalListModel::listEdited(
    const ComponentSignalList& list, int index,
    const std::shared_ptr<const ComponentSignal>& item,
    ComponentSignalList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case ComponentSignalList::Event::ElementAdded:
      mItems.insert(index, createItem(*item, index));
      notify_row_added(index, 1);
      updateSortOrder(true);
      break;
    case ComponentSignalList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case ComponentSignalList::Event::ElementEdited:
      mItems[index] = createItem(*item, index);
      notify_row_changed(index);
      updateSortOrder(true);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentSignalListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void ComponentSignalListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

CircuitIdentifier ComponentSignalListModel::validateNameOrThrow(
    const QString& name) const {
  if (mComponent && mComponent->getSignals().contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("There is already a signal with the name \"%1\".").arg(name));
  }
  return CircuitIdentifier(name);  // can throw
}

QString ComponentSignalListModel::cleanForcedNetName(
    const QString& name) noexcept {
  // Same as cleanCircuitIdentifier(), but allowing '{' and '}' because it's
  // allowed to have attribute placeholders in a forced net name. Also remove
  // spaces because they must not be replaced by underscores inside {{ and }}.
  return Toolbox::cleanUserInputString(
      name, QRegularExpression("[^-a-zA-Z0-9_+/!?@#$\\{\\}]"), true, false,
      false, "");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
