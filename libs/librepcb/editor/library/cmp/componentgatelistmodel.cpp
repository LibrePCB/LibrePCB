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

#include "../../graphics/graphicsscene.h"
#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../cmd/cmdcomponentsignaledit.h"
#include "../cmd/cmdcomponentsymbolvariantitemedit.h"
#include "../libraryelementcache.h"
#include "../sym/symbolchooserdialog.h"
#include "componentgateeditor.h"
#include "componentsignalnamelistmodel.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/sym/symbol.h>

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

ComponentGateListModel::ComponentGateListModel(const Workspace& ws,
                                               const GraphicsLayerList& layers,
                                               const LibraryElementCache& cache,
                                               QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLayers(layers),
    mCache(cache),
    mComponent(nullptr),
    mComponentScene(nullptr),
    mList(nullptr),
    mUndoStack(nullptr),
    mWizardMode(nullptr),
    mOnEditedSlot(*this, &ComponentGateListModel::listEdited) {
}

ComponentGateListModel::~ComponentGateListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentGateListModel::setReferences(
    ComponentSymbolVariantItemList* list, QPointer<Component> component,
    QPointer<GraphicsScene> componentScene,
    const std::shared_ptr<ComponentSignalNameListModel>& sigs, UndoStack* stack,
    const bool* wizardMode) noexcept {
  mComponentScene = componentScene;

  if ((list == mList) && (component == mComponent) && (stack == mUndoStack)) {
    return;
  }

  mComponent = component;
  mSignals = sigs;
  mUndoStack = stack;
  mWizardMode = wizardMode;

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mItems.clear();

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);

    for (auto gate : mList->values()) {
      auto editor = std::make_shared<ComponentGateEditor>(
          mWorkspace, mLayers, mCache,
          const_cast<const Component*>(mComponent.get()), mComponentScene,
          mSignals, gate, mUndoStack);
      connect(editor.get(), &ComponentGateEditor::uiDataChanged, this,
              &ComponentGateListModel::gateUiDataChanged);
      mItems.append(editor);
    }
  }

  notify_reset();
}

slint::Image ComponentGateListModel::renderScene(int gate, float width,
                                                 float height) noexcept {
  if (auto editor = mItems.value(gate)) {
    return editor->renderScene(width, height);
  } else {
    return slint::Image();
  }
}

static QString appendNumberToSignalName(QString name, int number) noexcept {
  name.truncate(CircuitIdentifierConstraint::MAX_LENGTH - 4);
  if ((!name.isEmpty()) && (name.back().isDigit())) {
    name.append("_");
  }
  name.append(QString::number(number));
  return cleanCircuitIdentifier(name);
}

void ComponentGateListModel::add() {
  if ((!mList) || (!mComponent)) return;

  SymbolChooserDialog dialog(mWorkspace, mLayers, qApp->activeWindow());
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const auto symbolUuid = dialog.getSelectedSymbolUuid();
  if (!symbolUuid) {
    return;
  }

  std::shared_ptr<const Symbol> sym =
      mCache.getSymbol(*symbolUuid, true);  // can throw
  Q_ASSERT(sym);

  std::unique_ptr<UndoCommandGroup> cmdGrp(
      new UndoCommandGroup("Add Component Gate"));
  auto gate = std::make_shared<ComponentSymbolVariantItem>(
      Uuid::createRandom(), *symbolUuid, Point(), Angle(), true,
      ComponentSymbolVariantItemSuffix(""));
  for (auto pin : sym->getPins()) {
    std::optional<Uuid> sigUuid;
    if (mWizardMode && (*mWizardMode)) {
      QString name = *pin.getName();
      int number = 2;
      while (mComponent->getSignals().contains(name)) {
        // Append number to make the signal name unique.
        // (https://github.com/LibrePCB/LibrePCB/issues/1425).
        name = appendNumberToSignalName(*pin.getName(), number);
        ++number;
      }
      auto sig = std::make_shared<ComponentSignal>(
          Uuid::createRandom(), CircuitIdentifier(name), SignalRole::passive(),
          QString(), false, false, false);
      sigUuid = sig->getUuid();
      cmdGrp->appendChild(
          new CmdComponentSignalInsert(mComponent->getSignals(), sig));
    }
    gate->getPinSignalMap().append(std::make_shared<ComponentPinSignalMapItem>(
        pin.getUuid(), sigUuid, CmpSigPinDisplayType::componentSignal()));
  }
  cmdGrp->appendChild(new CmdComponentSymbolVariantItemInsert(*mList, gate));
  execCmd(cmdGrp.release(), true);
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentGateListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::ComponentGateData> ComponentGateListModel::row_data(
    std::size_t i) const {
  if (auto item = mItems.value(i)) {
    return item->getUiData();
  } else {
    return std::nullopt;
  }
}

void ComponentGateListModel::set_row_data(
    std::size_t i, const ui::ComponentGateData& data) noexcept {
  if (!mList) {
    return;
  }

  if (data.action != ui::ComponentGateAction::None) {
    if (auto obj = mList->value(i)) {
      // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
      QMetaObject::invokeMethod(
          this, [this, i, obj, a = data.action]() { trigger(i, obj, a); },
          Qt::QueuedConnection);
    }
  } else if (auto editor = mItems.value(i)) {
    editor->setUiData(data);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentGateListModel::trigger(
    int index, std::shared_ptr<ComponentSymbolVariantItem> obj,
    ui::ComponentGateAction a) noexcept {
  if ((!mList) || (!obj) || (mList->value(index) != obj)) {
    return;
  }

  try {
    if (a == ui::ComponentGateAction::MoveUp) {
      execCmd(new CmdComponentSymbolVariantItemsSwap(*mList, index, index - 1),
              true);
    } else if (a == ui::ComponentGateAction::Delete) {
      std::unique_ptr<UndoCommandGroup> cmdGrp(
          new UndoCommandGroup("Remove Component Gate"));
      cmdGrp->appendChild(
          new CmdComponentSymbolVariantItemRemove(*mList, obj.get()));
      if (mWizardMode && (*mWizardMode) && mComponent) {
        for (auto pinout : obj->getPinSignalMap()) {
          if (auto sigUuid = pinout.getSignalUuid()) {
            if (auto sig = mComponent->getSignals().find(*sigUuid)) {
              cmdGrp->appendChild(new CmdComponentSignalRemove(
                  mComponent->getSignals(), sig.get()));
            }
          }
        }
      }
      execCmd(cmdGrp.release(), true);
    } else if (a == ui::ComponentGateAction::ChooseSymbol) {
      if (auto editor = mItems.value(index)) {
        editor->chooseSymbol();
      }
    }
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void ComponentGateListModel::listEdited(
    const ComponentSymbolVariantItemList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariantItem>& item,
    ComponentSymbolVariantItemList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case ComponentSymbolVariantItemList::Event::ElementAdded: {
      auto editor = std::make_shared<ComponentGateEditor>(
          mWorkspace, mLayers, mCache,
          const_cast<const Component*>(mComponent.get()), mComponentScene,
          mSignals, std::const_pointer_cast<ComponentSymbolVariantItem>(item),
          mUndoStack);
      connect(editor.get(), &ComponentGateEditor::uiDataChanged, this,
              &ComponentGateListModel::gateUiDataChanged);
      mItems.insert(index, editor);
      notify_row_added(index, 1);
      break;
    }
    case ComponentSymbolVariantItemList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case ComponentSymbolVariantItemList::Event::ElementEdited:
      if (auto editor = mItems.value(index)) {
        editor->refreshPreview();  // Calls notify_row_changed() if needed.
      }
      break;
    default:
      qWarning()
          << "Unhandled switch-case in ComponentGateListModel::listEdited():"
          << static_cast<int>(event);
      break;
  }
}

void ComponentGateListModel::gateUiDataChanged() noexcept {
  for (int i = 0; i < mItems.count(); ++i) {
    if (mItems[i].get() == sender()) {
      notify_row_changed(i);
    }
  }
}

void ComponentGateListModel::execCmd(UndoCommand* cmd, bool updateSuffixes) {
  // Apply automatic suffix only if it already conforms to our system.
  const QString suffixes = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int i = 0; updateSuffixes && mList && (i < mList->count()); ++i) {
    const QString expected =
        (mList->count() == 1) ? QString() : suffixes.mid(i, 1);
    updateSuffixes = (mList->at(i)->getSuffix() == expected);
  }

  if (mUndoStack) {
    UndoStackTransaction transaction(*mUndoStack, cmd->getText());
    transaction.append(cmd);
    if (updateSuffixes) {
      transaction.append(createSuffixUpdateCmd().release());
    }
    transaction.commit();
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
    if (updateSuffixes) {
      createSuffixUpdateCmd()->execute();
    }
  }
}

std::unique_ptr<UndoCommandGroup>
    ComponentGateListModel::createSuffixUpdateCmd() {
  // See https://github.com/LibrePCB/LibrePCB/issues/1426.
  std::unique_ptr<UndoCommandGroup> cmdGroup(
      new UndoCommandGroup("Update symbol suffixes"));
  const QString suffixes = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int i = 0; (mList) && (i < mList->count()); ++i) {
    std::shared_ptr<ComponentSymbolVariantItem> item = mList->value(i);
    const QString suffix =
        (mList->count() == 1) ? QString() : suffixes.mid(i, 1);
    std::unique_ptr<CmdComponentSymbolVariantItemEdit> cmd(
        new CmdComponentSymbolVariantItemEdit(*item));
    cmd->setSuffix(ComponentSymbolVariantItemSuffix(
        cleanComponentSymbolVariantItemSuffix(suffix)));
    cmdGroup->appendChild(cmd.release());
  }
  return cmdGroup;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
