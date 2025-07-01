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
#include "componentgatelistmodel.h"

#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdcomponentsymbolvariantitemedit.h"

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

ComponentGateListModel::ComponentGateListModel(QObject* parent) noexcept
  : QObject(parent),
    mGateList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &ComponentGateListModel::variantListEdited) {
}

ComponentGateListModel::~ComponentGateListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentGateListModel::setGateList(
    ComponentSymbolVariantItemList* list) noexcept {
  if (list == mGateList) return;

  if (mGateList) {
    mGateList->onEdited.detach(mOnEditedSlot);
  }

  mGateList = list;
  mItems.clear();

  if (mGateList) {
    mGateList->onEdited.attach(mOnEditedSlot);

    for (auto sig : *mGateList) {
      mItems.append(createItem(sig));
    }
  }

  notify_reset();
}

void ComponentGateListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*bool ComponentGateListModel::add(const QStringList& names) noexcept {
  if (!mGateList) return false;

  try {
    std::unique_ptr<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add Component Signal(s)")));
    foreach (const QString& nameStr, names) {
      const CircuitIdentifier name(
          cleanCircuitIdentifier(nameStr));  // can throw
      if (mGateList->contains(*name)) {
        throwDuplicateNameError(*name);
      }
      std::shared_ptr<ComponentSignal> sig = std::make_shared<ComponentSignal>(
          Uuid::createRandom(), name, SignalRole::passive(), QString(), false,
          false, false);
      cmd->appendChild(new CmdComponentSignalInsert(*mGateList, sig));
    }
    execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}*/

void ComponentGateListModel::apply() {
  if (!mGateList) return;

  for (auto i = 0; i < mItems.count(); ++i) {
    auto& item = mItems[i];
    if (auto gate = mGateList->value(i)) {
      std::unique_ptr<CmdComponentSymbolVariantItemEdit> cmd(
          new CmdComponentSymbolVariantItemEdit(*gate));
      /*const auto name =
          parseCircuitIdentifier(cleanCircuitIdentifier(s2q(item.name)));
      if (name && ((*name) != sig->getName())) {
        if (mGateList->contains(**name)) {
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
      cmd->setForcedNetName(cleanForcedNetName(s2q(item.forced_net_name)));*/
      execCmd(cmd.release());
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentGateListModel::row_count() const {
  return mGateList ? mGateList->count() : 0;
}

std::optional<ui::ComponentGateData> ComponentGateListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void ComponentGateListModel::set_row_data(
    std::size_t i, const ui::ComponentGateData& data) noexcept {
  if (mGateList && (i < static_cast<std::size_t>(mItems.count()))) {
    /*if (auto variant = mGateList->value(i)) {
      if (data.delete_) {
        // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
        QMetaObject::invokeMethod(
            this,
            [this, sig]() {
              try {
                execCmd(new CmdComponentSignalRemove(*mGateList, sig.get()));
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
    }*/
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::ComponentGateData ComponentGateListModel::createItem(
    const ComponentSymbolVariantItem& item) noexcept {
  return ui::ComponentGateData{
      q2s(item.getUuid().toStr().left(8)),  // ID
      slint::SharedString(),  // Symbol name
      l2s(item.getSymbolPosition().getX()),  // Symbol X
      l2s(item.getSymbolPosition().getY()),  // Symbol Y
      l2s(item.getSymbolRotation()),  // Symbol rotation
      item.isRequired(),  // Required
      q2s(*item.getSuffix()),  // Suffix
      nullptr,  // Pinout
      slint::Image(),  // Image
  };
}

void ComponentGateListModel::variantListEdited(
    const ComponentSymbolVariantItemList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariantItem>& item,
    ComponentSymbolVariantItemList::Event event) noexcept {
  Q_UNUSED(list);
  switch (event) {
    case ComponentSymbolVariantItemList::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case ComponentSymbolVariantItemList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case ComponentSymbolVariantItemList::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentGateListModel::signalListEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void ComponentGateListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

void ComponentGateListModel::throwDuplicateNameError(const QString& name) {
  throw RuntimeError(
      __FILE__, __LINE__,
      tr("There is already a variant with the name \"%1\".").arg(name));
}

QString ComponentGateListModel::cleanForcedNetName(
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
