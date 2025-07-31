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
#include "componentvarianteditor.h"

#include "../../graphics/graphicsscene.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdcomponentpinsignalmapitemedit.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "../libraryelementcache.h"
#include "componentgatelistmodel.h"
#include "componentsignalnamelistmodel.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentsignal.h>
#include <librepcb/core/library/cmp/componentsymbolvariant.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

ComponentVariantEditor::ComponentVariantEditor(
    const Workspace& ws, const GraphicsLayerList& layers,
    const LibraryElementCache& cache, QPointer<Component> component,
    const std::shared_ptr<ComponentSignalNameListModel>& sigs,
    std::shared_ptr<ComponentSymbolVariant> variant, UndoStack* stack,
    const bool* wizardMode, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLayers(layers),
    mCache(cache),
    mComponent(component),
    mVariant(variant),
    mUndoStack(stack),
    mWizardMode(wizardMode),
    mScene(new GraphicsScene()),
    mFrameIndex(0),
    mGates(new ComponentGateListModel(ws, layers, cache)),
    mHasUnassignedSignals(false) {
  mGates->setReferences(&mVariant->getSymbolItems(), component, mScene.get(),
                        sigs, mUndoStack, mWizardMode);
  connect(sigs.get(), &ComponentSignalNameListModel::modified, this,
          &ComponentVariantEditor::updateUnassignedSignals);
  connect(mScene.get(), &GraphicsScene::changed, this, [this]() {
    ++mFrameIndex;
    emit uiDataChanged();
  });
  updateUnassignedSignals();
}

ComponentVariantEditor::~ComponentVariantEditor() noexcept {
  mGates->setReferences(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::ComponentVariantData ComponentVariantEditor::getUiData() const {
  return ui::ComponentVariantData{
      q2s(mVariant->getUuid().toStr().left(8)),  // ID
      q2s(*mVariant->getNames().getDefaultValue()),  // Name
      q2s(mVariant->getDescriptions().getDefaultValue()),  // Description
      q2s(mVariant->getNorm()),  // Norm
      mGates,  // Gates
      mHasUnassignedSignals,  // Has unassigned signals
      ui::ComponentVariantAction::None,  // Action
      mFrameIndex,  // Frame index
  };
}

void ComponentVariantEditor::setUiData(
    const ui::ComponentVariantData& data) noexcept {
  try {
    const QString name = s2q(data.name);
    const QString description = s2q(data.description);
    const QString norm = s2q(data.norm);

    std::unique_ptr<CmdComponentSymbolVariantEdit> cmd(
        new CmdComponentSymbolVariantEdit(*mVariant));
    if (name != mVariant->getNames().getDefaultValue()) {
      if (auto parsed = parseElementName(cleanElementName(name))) {
        if (mComponent && mComponent->getSymbolVariants().contains(**parsed)) {
          throw RuntimeError(
              __FILE__, __LINE__,
              tr("There is already a variant with the name \"%1\".").arg(name));
        }
        auto names = mVariant->getNames();
        names.setDefaultValue(*parsed);
        cmd->setNames(names);
      }
    }
    if (description != mVariant->getDescriptions().getDefaultValue()) {
      auto descriptions = mVariant->getDescriptions();
      descriptions.setDefaultValue(description.trimmed());
      cmd->setDescriptions(descriptions);
    }
    if (norm != mVariant->getNorm()) {
      cmd->setNorm(norm.trimmed());
    }
    execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

slint::Image ComponentVariantEditor::renderScene(int gate, float width,
                                                 float height) noexcept {
  if (gate == static_cast<int>(mGates->row_count())) {
    SlintGraphicsView view(SlintGraphicsView::defaultSymbolSceneRect());
    view.setUseOpenGl(mWorkspace.getSettings().useOpenGl.get());
    return view.render(*mScene, width, height);
  } else {
    return mGates->renderScene(gate, width, height);
  }
}

void ComponentVariantEditor::addGate() {
  mGates->add();
}

void ComponentVariantEditor::autoConnectPins() {
  if (!mComponent) {
    return;
  }

  QHash<QString, int> numbers;  // Memory how many ambiguous names were used.
  std::unique_ptr<UndoCommandGroup> cmdGrp(
      new UndoCommandGroup(tr("Auto-Assign Component Signals")));
  for (ComponentSymbolVariantItem& item : mVariant->getSymbolItems()) {
    if (std::shared_ptr<const Symbol> symbol =
            mCache.getSymbol(item.getSymbolUuid(), false)) {
      for (auto mapItem : item.getPinSignalMap().values()) {
        CircuitIdentifier pinName =
            symbol->getPins().get(mapItem->getPinUuid())->getName();
        std::shared_ptr<const ComponentSignal> signal =
            mComponent->getSignals().find(*pinName);
        if (!signal) {
          // Also look for names with a number at the end.
          int number = numbers.value(*pinName, 0) + 1;
          signal = mComponent->getSignals().find(
              appendNumberToSignalName(*pinName, number));
          if (signal) {
            numbers[*pinName] = number;
          }
        }
        std::optional<Uuid> signalUuid =
            signal ? std::make_optional(signal->getUuid()) : std::nullopt;
        std::unique_ptr<CmdComponentPinSignalMapItemEdit> cmd(
            new CmdComponentPinSignalMapItemEdit(mapItem));
        cmd->setSignalUuid(signalUuid);
        cmdGrp->appendChild(cmd.release());  // can throw
      }
    }
  }
  execCmd(cmdGrp.release());  // can throw
}

void ComponentVariantEditor::updateUnassignedSignals() noexcept {
  mHasUnassignedSignals = false;

  if (mComponent) {
    QSet<Uuid> connectedSignals;
    int unconnectedPins = 0;
    for (const auto& gate : mVariant->getSymbolItems()) {
      for (const auto& pinout : gate.getPinSignalMap()) {
        if (auto uuid = pinout.getSignalUuid()) {
          connectedSignals.insert(*uuid);
        } else {
          ++unconnectedPins;
        }
      }
    }
    mHasUnassignedSignals = (unconnectedPins > 0) &&
        (!(mComponent->getSignals().getUuidSet() - connectedSignals).isEmpty());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentVariantEditor::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

QString ComponentVariantEditor::appendNumberToSignalName(QString name,
                                                         int number) noexcept {
  name.truncate(CircuitIdentifierConstraint::MAX_LENGTH - 4);
  if ((!name.isEmpty()) && (name.back().isDigit())) {
    name.append("_");
  }
  name.append(QString::number(number));
  return cleanCircuitIdentifier(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
