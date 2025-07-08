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
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdcomponentsignaledit.h"

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
    mSignalList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &ComponentSignalListModel::signalListEdited) {
}

ComponentSignalListModel::~ComponentSignalListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSignalListModel::setSignalList(
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

void ComponentSignalListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

bool ComponentSignalListModel::add(const QStringList& names) noexcept {
  if (!mSignalList) return false;

  try {
    std::unique_ptr<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add Component Signal(s)")));
    foreach (const QString& nameStr, names) {
      const CircuitIdentifier name(
          cleanCircuitIdentifier(nameStr));  // can throw
      if (mSignalList->contains(*name)) {
        throwDuplicateNameError(*name);
      }
      std::shared_ptr<ComponentSignal> sig = std::make_shared<ComponentSignal>(
          Uuid::createRandom(), name, SignalRole::passive(), QString(), false,
          false, false);
      cmd->appendChild(new CmdComponentSignalInsert(*mSignalList, sig));
    }
    execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}

void ComponentSignalListModel::apply() {
  if (!mSignalList) return;

  for (auto i = 0; i < mItems.count(); ++i) {
    auto& item = mItems[i];
    if (auto sig = mSignalList->value(i)) {
      std::unique_ptr<CmdComponentSignalEdit> cmd(
          new CmdComponentSignalEdit(*sig));
      const auto name =
          parseCircuitIdentifier(cleanCircuitIdentifier(s2q(item.name)));
      if (name && ((*name) != sig->getName())) {
        if (mSignalList->contains(**name)) {
          item.name = q2s(*sig->getName());
          item.name_error = slint::SharedString();
          notify_row_changed(i);
          throwDuplicateNameError(**name);
        }
        cmd->setName(*name);
      } else {
        item.name = q2s(*sig->getName());
        item.name_error = slint::SharedString();
        notify_row_changed(i);
      }
      cmd->setIsRequired(item.required);
      cmd->setForcedNetName(cleanForcedNetName(s2q(item.forced_net_name)));
      execCmd(cmd.release());
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentSignalListModel::row_count() const {
  return mSignalList ? mSignalList->count() : 0;
}

std::optional<ui::ComponentSignalData> ComponentSignalListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void ComponentSignalListModel::set_row_data(
    std::size_t i, const ui::ComponentSignalData& data) noexcept {
  if (mSignalList && (i < static_cast<std::size_t>(mItems.count()))) {
    if (auto sig = mSignalList->value(i)) {
      if (data.delete_) {
        // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
        QMetaObject::invokeMethod(
            this,
            [this, sig]() {
              try {
                execCmd(new CmdComponentSignalRemove(*mSignalList, sig.get()));
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
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::ComponentSignalData ComponentSignalListModel::createItem(
    const ComponentSignal& sig) noexcept {
  return ui::ComponentSignalData{
      q2s(sig.getUuid().toStr().left(8)),  // ID
      q2s(*sig.getName()),  // Name
      slint::SharedString(),  // Name error
      q2s(sig.getForcedNetName()),  // Forced net name
      sig.isRequired(),  // Required
      false,  // Delete
  };
}

void ComponentSignalListModel::signalListEdited(
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
                    "ComponentSignalListModel::signalListEdited():"
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

void ComponentSignalListModel::throwDuplicateNameError(const QString& name) {
  throw RuntimeError(
      __FILE__, __LINE__,
      tr("There is already a signal with the name \"%1\".").arg(name));
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
