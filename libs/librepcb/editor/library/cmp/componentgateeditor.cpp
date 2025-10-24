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
#include "componentgateeditor.h"

#include "../../graphics/graphicsscene.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdcomponentpinsignalmapitemedit.h"
#include "../cmd/cmdcomponentsymbolvariantitemedit.h"
#include "../libraryelementcache.h"
#include "../sym/symbolchooserdialog.h"
#include "../sym/symbolgraphicsitem.h"
#include "componentpinoutlistmodel.h"
#include "componentsignalnamelistmodel.h"

#include <librepcb/core/library/cmp/componentsymbolvariantitem.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentGateEditor::ComponentGateEditor(
    const Workspace& ws, const GraphicsLayerList& layers,
    const LibraryElementCache& cache, QPointer<const Component> component,
    QPointer<GraphicsScene> componentScene,
    const std::shared_ptr<ComponentSignalNameListModel>& sigs,
    std::shared_ptr<ComponentSymbolVariantItem> gate, UndoStack* stack,
    QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLayers(layers),
    mCache(cache),
    mComponent(component),
    mComponentScene(componentScene),
    mSignals(sigs),
    mGate(gate),
    mUndoStack(stack),
    mFrameIndex(0),
    mPinout(new ComponentPinoutListModel()),
    mPinoutSorted(new slint::SortModel<ui::ComponentPinoutData>(
        mPinout,
        [this](const ui::ComponentPinoutData& a,
               const ui::ComponentPinoutData& b) {
          return mCollator(
              EditorToolbox::sortableCircuitIdentifier(s2q(a.pin_name)),
              EditorToolbox::sortableCircuitIdentifier(s2q(b.pin_name)));
        })) {
  mCollator.setNumericMode(true);
  mCollator.setCaseSensitivity(Qt::CaseInsensitive);
  mCollator.setIgnorePunctuation(false);

  // Refresh pinout when signal names have changed.
  connect(mSignals.get(), &ComponentSignalNameListModel::modified, this,
          &ComponentGateEditor::refreshPreview);

  refreshPreview();
}

ComponentGateEditor::~ComponentGateEditor() noexcept {
  mPinout->setReferences(nullptr, nullptr, nullptr, nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::ComponentGateData ComponentGateEditor::getUiData() const {
  const QString name = mSymbol ? *mSymbol->getNames().getDefaultValue()
                               : mGate->getSymbolUuid().toStr();

  return ui::ComponentGateData{
      q2s(mGate->getUuid().toStr().left(8)),  // ID
      q2s(name),  // Symbol name
      l2s(mGate->getSymbolPosition().getX()),  // Symbol X
      l2s(mGate->getSymbolPosition().getY()),  // Symbol Y
      l2s(mGate->getSymbolRotation()),  // Symbol rotation
      mGate->isRequired(),  // Required
      q2s(*mGate->getSuffix()),  // Suffix
      mPinoutSorted,  // Pinout
      ui::ComponentGateAction::None,  // Action
      mFrameIndex,  // Frame index
  };
}

void ComponentGateEditor::setUiData(
    const ui::ComponentGateData& data) noexcept {
  try {
    const QString suffix = s2q(data.suffix);

    std::unique_ptr<CmdComponentSymbolVariantItemEdit> cmd(
        new CmdComponentSymbolVariantItemEdit(*mGate));
    if (suffix != mGate->getSuffix()) {
      cmd->setSuffix(ComponentSymbolVariantItemSuffix(
          cleanComponentSymbolVariantItemSuffix(suffix)));
    }
    cmd->setIsRequired(data.required);
    cmd->setSymbolPosition(
        Point(s2length(data.symbol_x), s2length(data.symbol_y)));
    cmd->setSymbolRotation(s2angle(data.symbol_rotation));
    execCmd(cmd.release());
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

slint::Image ComponentGateEditor::renderScene(float width,
                                              float height) noexcept {
  if (mScene) {
    SlintGraphicsView view(SlintGraphicsView::defaultSymbolSceneRect());
    view.setUseOpenGl(mWorkspace.getSettings().useOpenGl.get());
    return view.render(*mScene, width, height);
  } else {
    return slint::Image();
  }
}

void ComponentGateEditor::chooseSymbol() {
  SymbolChooserDialog dialog(mWorkspace, mLayers, qApp->activeWindow());
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const auto symbolUuid = dialog.getSelectedSymbolUuid();
  if ((!symbolUuid) || (symbolUuid == mGate->getSymbolUuid())) {
    return;
  }

  std::shared_ptr<const Symbol> sym =
      mCache.getSymbol(*symbolUuid, true);  // can throw
  Q_ASSERT(sym);

  std::unique_ptr<UndoCommandGroup> cmdGrp(
      new UndoCommandGroup(tr("Edit Component Gate")));
  std::unique_ptr<CmdComponentSymbolVariantItemEdit> cmd(
      new CmdComponentSymbolVariantItemEdit(*mGate));
  cmd->setSymbolUuid(*symbolUuid);
  cmdGrp->appendChild(cmd.release());
  for (int i = 0; i < mGate->getPinSignalMap().count(); ++i) {
    cmdGrp->appendChild(new CmdComponentPinSignalMapItemRemove(
        mGate->getPinSignalMap(), mGate->getPinSignalMap().value(i).get()));
  }
  auto newPinout =
      ComponentPinSignalMapHelpers::create(sym->getPins().getUuidSet());
  for (auto item : newPinout.values()) {
    cmdGrp->appendChild(
        new CmdComponentPinSignalMapItemInsert(mGate->getPinSignalMap(), item));
  }
  execCmd(cmdGrp.release());
}

void ComponentGateEditor::refreshPreview() noexcept {
  if (mSymbol && mGraphicsItem && mComponentGraphicsItem &&
      (mSymbol->getUuid() == mGate->getSymbolUuid())) {
    mGraphicsItem->updateAllTexts();
    mComponentGraphicsItem->updateAllTexts();
    mComponentGraphicsItem->setPosition(mGate->getSymbolPosition());
    mComponentGraphicsItem->setRotation(mGate->getSymbolRotation());
    ++mFrameIndex;
    emit uiDataChanged();
    return;
  }

  reloadSymbol();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentGateEditor::reloadSymbol() noexcept {
  disconnect(mCacheConnection);
  mComponentGraphicsItem.reset();
  mGraphicsItem.reset();
  mScene.reset();
  mSymbol = mCache.getSymbol(mGate->getSymbolUuid(), false);  // fail silently

  if (mSymbol) {
    mScene.reset(new GraphicsScene());
    mScene->setOriginCrossVisible(false);  // It's rather disruptive.
    mGraphicsItem.reset(new SymbolGraphicsItem(
        const_cast<Symbol&>(*mSymbol), mLayers, mComponent, mGate,
        mWorkspace.getSettings().libraryLocaleOrder.get(), false));
    mScene->addItem(*mGraphicsItem);
    mComponentGraphicsItem.reset(new SymbolGraphicsItem(
        const_cast<Symbol&>(*mSymbol), mLayers, mComponent, mGate,
        mWorkspace.getSettings().libraryLocaleOrder.get(), false));
    mComponentGraphicsItem->setPosition(mGate->getSymbolPosition());
    mComponentGraphicsItem->setRotation(mGate->getSymbolRotation());
    if (mComponentScene) {
      mComponentScene->addItem(*mComponentGraphicsItem);
    }

    // If the symbol was (potentially) modified, reload it.
    mCacheConnection =
        connect(&mCache, &LibraryElementCache::scanStarted, this,
                &ComponentGateEditor::reloadSymbol, Qt::QueuedConnection);
  } else {
    // If the symbol was (potentially) installed, reload it.
    mCacheConnection =
        connect(&mCache, &LibraryElementCache::scanSucceeded, this,
                &ComponentGateEditor::reloadSymbol, Qt::QueuedConnection);
  }

  mPinout->setReferences(&mGate->getPinSignalMap(),
                         mSymbol ? &mSymbol->getPins() : nullptr, mSignals,
                         mUndoStack);
  ++mFrameIndex;
  emit uiDataChanged();
}

void ComponentGateEditor::execCmd(UndoCommand* cmd) {
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
