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
#include "componentvariantlistmodel.h"

#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "componentgatelistmodel.h"
#include "componentvarianteditor.h"

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

ComponentVariantListModel::ComponentVariantListModel(
    const Workspace& ws, const GraphicsLayerList& layers,
    QPointer<const Component> component, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLayers(layers),
    mComponent(component),
    mVariantList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &ComponentVariantListModel::variantListEdited) {
}

ComponentVariantListModel::~ComponentVariantListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentVariantListModel::setVariantList(
    ComponentSymbolVariantList* list) noexcept {
  if (list == mVariantList) return;

  if (mVariantList) {
    mVariantList->onEdited.detach(mOnEditedSlot);
  }

  mVariantList = list;
  mItems.clear();

  if (mVariantList) {
    mVariantList->onEdited.attach(mOnEditedSlot);

    for (auto variant : mVariantList->values()) {
      mItems.append(std::make_shared<ComponentVariantEditor>(
          mWorkspace, mLayers, mComponent, variant));
    }
  }

  notify_reset();
}

void ComponentVariantListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

slint::Image ComponentVariantListModel::renderScene(int variant, int gate,
                                                    float width,
                                                    float height) noexcept {
  if (auto editor = mItems.value(variant)) {
    return editor->renderScene(gate, width, height);
  } else {
    return slint::Image();
  }
}

/*bool ComponentVariantListModel::add(const QStringList& names) noexcept {
  if (!mVariantList) return false;

  try {
    std::unique_ptr<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add Component Signal(s)")));
    foreach (const QString& nameStr, names) {
      const CircuitIdentifier name(
          cleanCircuitIdentifier(nameStr));  // can throw
      if (mVariantList->contains(*name)) {
        throwDuplicateNameError(*name);
      }
      std::shared_ptr<ComponentSignal> sig = std::make_shared<ComponentSignal>(
          Uuid::createRandom(), name, SignalRole::passive(), QString(), false,
          false, false);
      cmd->appendChild(new CmdComponentSignalInsert(*mVariantList, sig));
    }
    execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}*/

void ComponentVariantListModel::apply() {
  if (!mVariantList) return;

  for (auto i = 0; i < mItems.count(); ++i) {
    auto& item = mItems[i];
    if (auto variant = mVariantList->value(i)) {
      std::unique_ptr<CmdComponentSymbolVariantEdit> cmd(
          new CmdComponentSymbolVariantEdit(*variant));
      /*const auto name =
          parseCircuitIdentifier(cleanCircuitIdentifier(s2q(item.name)));
      if (name && ((*name) != sig->getName())) {
        if (mVariantList->contains(**name)) {
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

std::size_t ComponentVariantListModel::row_count() const {
  return mVariantList ? mVariantList->count() : 0;
}

std::optional<ui::ComponentVariantData> ComponentVariantListModel::row_data(
    std::size_t i) const {
  if (auto editor = mItems.value(i)) {
    return editor->getUiData();
  } else {
    return std::nullopt;
  }
}

void ComponentVariantListModel::set_row_data(
    std::size_t i, const ui::ComponentVariantData& data) noexcept {
  if (mVariantList && (i < static_cast<std::size_t>(mItems.count()))) {
    /*if (auto variant = mVariantList->value(i)) {
      if (data.delete_) {
        // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
        QMetaObject::invokeMethod(
            this,
            [this, sig]() {
              try {
                execCmd(new CmdComponentSignalRemove(*mVariantList, sig.get()));
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

void ComponentVariantListModel::variantListEdited(
    const ComponentSymbolVariantList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariant>& variant,
    ComponentSymbolVariantList::Event event) noexcept {
  Q_UNUSED(list);
  switch (event) {
    case ComponentSymbolVariantList::Event::ElementAdded:
      mItems.insert(
          index,
          std::make_shared<ComponentVariantEditor>(
              mWorkspace, mLayers, mComponent,
              std::const_pointer_cast<ComponentSymbolVariant>(variant)));
      notify_row_added(index, 1);
      break;
    case ComponentSymbolVariantList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case ComponentSymbolVariantList::Event::ElementEdited:
      // mItems[index] =
      //     createItem(*std::const_pointer_cast<ComponentSymbolVariant>(variant));
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "ComponentVariantListModel::signalListEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void ComponentVariantListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

void ComponentVariantListModel::throwDuplicateNameError(const QString& name) {
  throw RuntimeError(
      __FILE__, __LINE__,
      tr("There is already a variant with the name \"%1\".").arg(name));
}

QString ComponentVariantListModel::cleanForcedNetName(
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
