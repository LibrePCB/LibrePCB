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
#include "../../../widgets/graphicsview.h"

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
  connect(mTool.data(), &MeasureTool::infoBoxTextChanged,
          &mContext.graphicsView, &GraphicsView::setInfoBoxText);
  connect(mTool.data(), &MeasureTool::statusBarMessageChanged, this,
          &SymbolEditorState_Measure::statusBarMessageChanged);
}

SymbolEditorState_Measure::~SymbolEditorState_Measure() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_Measure::entry() noexcept {
  mTool->setSymbol(&mContext.symbol);
  mTool->enter(mContext.graphicsScene, getLengthUnit(),
               mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos()));
  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool SymbolEditorState_Measure::exit() noexcept {
  mTool->leave();
  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    SymbolEditorState_Measure::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
      EditorWidgetBase::Feature::Copy,
      EditorWidgetBase::Feature::Remove,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_Measure::processKeyPressed(const QKeyEvent& e) noexcept {
  return mTool->processKeyPressed(e);
}

bool SymbolEditorState_Measure::processKeyReleased(
    const QKeyEvent& e) noexcept {
  return mTool->processKeyReleased(e);
}

bool SymbolEditorState_Measure::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  return mTool->processGraphicsSceneMouseMoved(e);
}

bool SymbolEditorState_Measure::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  return mTool->processGraphicsSceneLeftMouseButtonPressed(e);
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
