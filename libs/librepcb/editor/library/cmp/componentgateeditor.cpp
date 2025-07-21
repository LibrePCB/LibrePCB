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
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdcomponentpinsignalmapitemedit.h"
#include "../cmd/cmdcomponentsymbolvariantitemedit.h"
#include "../libraryelementcache.h"
#include "../sym/symbolchooserdialog.h"
#include "../sym/symbolgraphicsitem.h"
#include "componentpinoutlistmodel.h"

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
    const std::shared_ptr<ComponentSignalNameListModel>& sigs,
    std::shared_ptr<ComponentSymbolVariantItem> item, UndoStack* stack,
    QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLayers(layers),
    mCache(cache),
    mComponent(component),
    mSignals(sigs),
    mItem(item),
    mUndoStack(stack),
    mFrameIndex(0),
    mPinout(new ComponentPinoutListModel()),
    mPinoutSorted(new slint::SortModel<ui::ComponentPinoutData>(
        mPinout,
        [this](const ui::ComponentPinoutData& a,
               const ui::ComponentPinoutData& b) {
          return mCollator(a.pin_name.data(), b.pin_name.data());
        })) {
  mCollator.setNumericMode(true);
  mCollator.setCaseSensitivity(Qt::CaseInsensitive);
  mCollator.setIgnorePunctuation(false);

  refresh();
}

ComponentGateEditor::~ComponentGateEditor() noexcept {
  mPinout->setReferences(nullptr, nullptr, nullptr, nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::ComponentGateData ComponentGateEditor::getUiData() const {
  const QString name = mSymbol ? *mSymbol->getNames().getDefaultValue()
                               : mItem->getSymbolUuid().toStr();

  return ui::ComponentGateData{
      q2s(mItem->getUuid().toStr().left(8)),  // ID
      q2s(name),  // Symbol name
      l2s(mItem->getSymbolPosition().getX()),  // Symbol X
      l2s(mItem->getSymbolPosition().getY()),  // Symbol Y
      l2s(mItem->getSymbolRotation()),  // Symbol rotation
      mItem->isRequired(),  // Required
      q2s(*mItem->getSuffix()),  // Suffix
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
        new CmdComponentSymbolVariantItemEdit(*mItem));
    if (suffix != mItem->getSuffix()) {
      cmd->setSuffix(ComponentSymbolVariantItemSuffix(
          cleanComponentSymbolVariantItemSuffix(suffix)));
    }
    cmd->setIsRequired(data.required);
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
  if ((!symbolUuid) || (symbolUuid == mItem->getSymbolUuid())) {
    return;
  }

  std::shared_ptr<const Symbol> sym =
      mCache.getSymbol(*symbolUuid, true);  // can throw
  Q_ASSERT(sym);

  std::unique_ptr<UndoCommandGroup> cmdGrp(
      new UndoCommandGroup(tr("Edit Component Gate")));
  std::unique_ptr<CmdComponentSymbolVariantItemEdit> cmd(
      new CmdComponentSymbolVariantItemEdit(*mItem));
  cmd->setSymbolUuid(*symbolUuid);
  cmdGrp->appendChild(cmd.release());
  for (int i = 0; i < mItem->getPinSignalMap().count(); ++i) {
    cmdGrp->appendChild(new CmdComponentPinSignalMapItemRemove(
        mItem->getPinSignalMap(), mItem->getPinSignalMap().value(i).get()));
  }
  auto newPinout =
      ComponentPinSignalMapHelpers::create(sym->getPins().getUuidSet());
  for (auto item : newPinout.values()) {
    cmdGrp->appendChild(
        new CmdComponentPinSignalMapItemInsert(mItem->getPinSignalMap(), item));
  }
  execCmd(cmdGrp.release());
}

void ComponentGateEditor::refresh() noexcept {
  ++mFrameIndex;

  if (mSymbol && mGraphicsItem &&
      (mSymbol->getUuid() == mItem->getSymbolUuid())) {
    mGraphicsItem->updateAllTexts();
    return;
  }

  mGraphicsItem.reset();
  mScene.reset();
  mSymbol = mCache.getSymbol(mItem->getSymbolUuid(), false);  // fail silently

  if (mSymbol) {
    mScene.reset(new GraphicsScene());
    mScene->setOriginCrossVisible(false);  // It's rather disruptive.
    mGraphicsItem.reset(new SymbolGraphicsItem(const_cast<Symbol&>(*mSymbol),
                                               mLayers, mComponent, mItem));
    mScene->addItem(*mGraphicsItem);
  }

  mPinout->setReferences(&mItem->getPinSignalMap(),
                         mSymbol ? &mSymbol->getPins() : nullptr, mSignals,
                         mUndoStack);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

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
