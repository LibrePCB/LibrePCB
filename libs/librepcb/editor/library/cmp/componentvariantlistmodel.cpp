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
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "../libraryelementcache.h"
#include "../sym/symbolchooserdialog.h"
#include "componentvarianteditor.h"

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

ComponentVariantListModel::ComponentVariantListModel(
    const Workspace& ws, const GraphicsLayerList& layers,
    const LibraryElementCache& cache, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLayers(layers),
    mCache(cache),
    mComponent(nullptr),
    mSignals(nullptr),
    mList(nullptr),
    mUndoStack(nullptr),
    mWizardMode(nullptr),
    mOnEditedSlot(*this, &ComponentVariantListModel::listEdited) {
}

ComponentVariantListModel::~ComponentVariantListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentVariantListModel::setReferences(
    ComponentSymbolVariantList* list, QPointer<Component> component,
    const std::shared_ptr<ComponentSignalNameListModel>& sigs, UndoStack* stack,
    const bool* wizardMode) noexcept {
  if ((list == mList) && (component == mComponent) && (sigs == mSignals) &&
      (stack == mUndoStack)) {
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

    for (auto variant : mList->values()) {
      auto editor = std::make_shared<ComponentVariantEditor>(
          mWorkspace, mLayers, mCache, mComponent, mSignals, variant,
          mUndoStack, mWizardMode);
      connect(editor.get(), &ComponentVariantEditor::uiDataChanged, this,
              &ComponentVariantListModel::variantUiDataChanged);
      mItems.append(editor);
    }
  }

  notify_reset();
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

void ComponentVariantListModel::add() noexcept {
  if (!mList) return;

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

  auto gate = std::make_shared<ComponentSymbolVariantItem>(
      Uuid::createRandom(), *symbolUuid, Point(), Angle(), true,
      ComponentSymbolVariantItemSuffix(""));
  gate->getPinSignalMap() =
      ComponentPinSignalMapHelpers::create(sym->getPins().getUuidSet());

  const QString nameStr =
      mList->isEmpty() ? "default" : tr("Variant %1").arg(mList->count() + 1);
  std::optional<ElementName> name = parseElementName(nameStr);
  if (!name) {
    name = ElementName(QString("Variant %1").arg(mList->count() + 1));
  }
  auto variant = std::make_shared<ComponentSymbolVariant>(
      Uuid::createRandom(), QString(), *name, QString());
  variant->getSymbolItems().append(gate);

  execCmd(new CmdComponentSymbolVariantInsert(*mList, variant));
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ComponentVariantListModel::row_count() const {
  return mItems.count();
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
  if (!mList) {
    return;
  }

  if (data.action != ui::ComponentVariantAction::None) {
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

void ComponentVariantListModel::trigger(
    int index, std::shared_ptr<ComponentSymbolVariant> obj,
    ui::ComponentVariantAction a) noexcept {
  if ((!mList) || (!obj) || (mList->value(index) != obj)) {
    return;
  }

  try {
    if (a == ui::ComponentVariantAction::MoveUp) {
      execCmd(new CmdComponentSymbolVariantsSwap(*mList, index, index - 1));
    } else if (a == ui::ComponentVariantAction::SetAsDefault) {
      std::unique_ptr<UndoCommandGroup> cmdGrp(
          new UndoCommandGroup(tr("Set Default Component Variant")));
      cmdGrp->appendChild(
          new CmdComponentSymbolVariantRemove(*mList, obj.get()));
      cmdGrp->appendChild(new CmdComponentSymbolVariantInsert(*mList, obj, 0));
      execCmd(cmdGrp.release());
    } else if (a == ui::ComponentVariantAction::Delete) {
      execCmd(new CmdComponentSymbolVariantRemove(*mList, obj.get()));
    } else if (auto editor = mItems.value(index)) {
      if (a == ui::ComponentVariantAction::AutoConnectPins) {
        editor->autoConnectPins();
      } else if (a == ui::ComponentVariantAction::AddGate) {
        editor->addGate();
      }
    }
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void ComponentVariantListModel::listEdited(
    const ComponentSymbolVariantList& list, int index,
    const std::shared_ptr<const ComponentSymbolVariant>& variant,
    ComponentSymbolVariantList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case ComponentSymbolVariantList::Event::ElementAdded: {
      auto editor = std::make_shared<ComponentVariantEditor>(
          mWorkspace, mLayers, mCache, mComponent, mSignals,
          std::const_pointer_cast<ComponentSymbolVariant>(variant), mUndoStack,
          mWizardMode);
      connect(editor.get(), &ComponentVariantEditor::uiDataChanged, this,
              &ComponentVariantListModel::variantUiDataChanged);
      mItems.insert(index, editor);
      notify_row_added(index, 1);
      break;
    }
    case ComponentSymbolVariantList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case ComponentSymbolVariantList::Event::ElementEdited:
      if (auto editor = mItems.value(index)) {
        editor->updateUnassignedSignals();
      }
      notify_row_changed(index);
      break;
    default:
      qWarning()
          << "Unhandled switch-case in ComponentVariantListModel::listEdited():"
          << static_cast<int>(event);
      break;
  }
}

void ComponentVariantListModel::variantUiDataChanged() noexcept {
  for (int i = 0; i < mItems.count(); ++i) {
    if (mItems[i].get() == sender()) {
      notify_row_changed(i);
    }
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
