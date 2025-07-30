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
#include "symboleditorstate_measure.h"

#include "../../../utils/measuretool.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorState_Measure::SymbolEditorState_Measure(
    const Context& context) noexcept
  : SymbolEditorState(context), mTool(new MeasureTool()) {
  connect(
      mTool.data(), &MeasureTool::infoBoxTextChanged, this,
      [this](const QString& text) { mAdapter.fsmSetViewInfoBoxText(text); });
  connect(mTool.data(), &MeasureTool::statusBarMessageChanged, this,
          [this](const QString& message, int timeoutMs) {
            mAdapter.fsmSetStatusBarMessage(message, timeoutMs);
          });
}

SymbolEditorState_Measure::~SymbolEditorState_Measure() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_Measure::entry() noexcept {
  GraphicsScene* scene = getGraphicsScene();
  if (!scene) return false;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  mTool->setSymbol(&mContext.symbol);
  mTool->enter(*scene, getLengthUnit(),
               mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos()));
  return true;
}

bool SymbolEditorState_Measure::exit() noexcept {
  mTool->leave();
  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_Measure::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mTool->processKeyPressed(e.key, e.modifiers);
}

bool SymbolEditorState_Measure::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mTool->processKeyReleased(e.key, e.modifiers);
}

bool SymbolEditorState_Measure::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mTool->processGraphicsSceneMouseMoved(e.scenePos, e.modifiers);
}

bool SymbolEditorState_Measure::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return mTool->processGraphicsSceneLeftMouseButtonPressed();
}

bool SymbolEditorState_Measure::processCopy() noexcept {
  return mTool->processCopy();
}

bool SymbolEditorState_Measure::processRemove() noexcept {
  return mTool->processRemove();
}

bool SymbolEditorState_Measure::processAbortCommand() noexcept {
  return mTool->processAbortCommand();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
