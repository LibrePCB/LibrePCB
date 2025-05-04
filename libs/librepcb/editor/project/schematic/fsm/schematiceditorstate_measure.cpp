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
#include "schematiceditorstate_measure.h"

#include "../../../utils/measuretool.h"
#include "../schematicgraphicsscene.h"

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

SchematicEditorState_Measure::SchematicEditorState_Measure(
    const Context& context) noexcept
  : SchematicEditorState(context), mTool(new MeasureTool()) {
  connect(
      mTool.data(), &MeasureTool::infoBoxTextChanged, this,
      [this](const QString& text) { mAdapter.fsmSetViewInfoBoxText(text); });
  connect(mTool.data(), &MeasureTool::statusBarMessageChanged, this,
          [this](const QString& message, int timeoutMs) {
            mAdapter.fsmSetStatusBarMessage(message, timeoutMs);
          });
}

SchematicEditorState_Measure::~SchematicEditorState_Measure() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_Measure::entry() noexcept {
  GraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  mTool->setSchematic(getActiveSchematic());
  mTool->enter(*scene, getLengthUnit(),
               mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos()));
  return true;
}

bool SchematicEditorState_Measure::exit() noexcept {
  mTool->leave();
  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_Measure::processCopy() noexcept {
  return mTool->processCopy();
}

bool SchematicEditorState_Measure::processRemove() noexcept {
  return mTool->processRemove();
}

bool SchematicEditorState_Measure::processAbortCommand() noexcept {
  return mTool->processAbortCommand();
}

bool SchematicEditorState_Measure::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mTool->processKeyPressed(e.key, e.modifiers);
}

bool SchematicEditorState_Measure::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mTool->processKeyReleased(e.key, e.modifiers);
}

bool SchematicEditorState_Measure::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mTool->processGraphicsSceneMouseMoved(e.scenePos, e.modifiers);
}

bool SchematicEditorState_Measure::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return mTool->processGraphicsSceneLeftMouseButtonPressed();
}

bool SchematicEditorState_Measure::processSwitchToSchematicPage(
    int index) noexcept {
  Q_UNUSED(index);
  return true;
}

void SchematicEditorState_Measure::processSwitchedSchematicPage() noexcept {
  mTool->leave();
  mTool->setSchematic(getActiveSchematic());
  if (GraphicsScene* scene = getActiveSchematicScene()) {
    mTool->enter(*scene, getLengthUnit(),
                 mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
