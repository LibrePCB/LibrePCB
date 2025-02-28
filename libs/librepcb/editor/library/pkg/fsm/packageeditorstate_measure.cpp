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
#include "packageeditorstate_measure.h"

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

PackageEditorState_Measure::PackageEditorState_Measure(
    Context& context) noexcept
  : PackageEditorState(context), mTool(new MeasureTool()) {
  connect(mTool.data(), &MeasureTool::infoBoxTextChanged,
          &mContext.graphicsView, &GraphicsView::setInfoBoxText);
  connect(mTool.data(), &MeasureTool::statusBarMessageChanged, this,
          &PackageEditorState_Measure::statusBarMessageChanged);
}

PackageEditorState_Measure::~PackageEditorState_Measure() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_Measure::entry() noexcept {
  mTool->setFootprint(mContext.currentFootprint.get());
  mTool->enter(mContext.graphicsScene, getLengthUnit(),
               mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos()));
  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_Measure::exit() noexcept {
  mTool->leave();
  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_Measure::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
      EditorWidgetBase::Feature::Copy,
      EditorWidgetBase::Feature::Remove,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_Measure::processKeyPressed(
    const QKeyEvent& e) noexcept {
  return mTool->processKeyPressed(e);
}

bool PackageEditorState_Measure::processKeyReleased(
    const QKeyEvent& e) noexcept {
  return mTool->processKeyReleased(e);
}

bool PackageEditorState_Measure::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  return mTool->processGraphicsSceneMouseMoved(e);
}

bool PackageEditorState_Measure::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  return mTool->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool PackageEditorState_Measure::processCopy() noexcept {
  return mTool->processCopy();
}

bool PackageEditorState_Measure::processRemove() noexcept {
  return mTool->processRemove();
}

bool PackageEditorState_Measure::processAbortCommand() noexcept {
  return mTool->processAbortCommand();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
