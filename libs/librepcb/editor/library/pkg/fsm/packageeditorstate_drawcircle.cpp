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
#include "packageeditorstate_drawcircle.h"

#include "../../../cmd/cmdcircleedit.h"
#include "../../../graphics/circlegraphicsitem.h"
#include "../../../undostack.h"
#include "../footprintgraphicsitem.h"

#include <librepcb/core/library/pkg/footprint.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_DrawCircle::PackageEditorState_DrawCircle(
    Context& context) noexcept
  : PackageEditorState(context),
    mCurrentProperties(
        Uuid::createRandom(),  // Not relevant
        Layer::topLegend(),  // Most important layer
        UnsignedLength(200000),  // Typical width according library conventions
        false,  // Fill is needed very rarely
        false,  // Avoid creating annoying grab areas "by accident"
        Point(),  // Center is not relevant
        PositiveLength(1)  // Diameter is not relevant
        ),
    mCurrentCircle(nullptr),
    mCurrentGraphicsItem(nullptr) {
}

PackageEditorState_DrawCircle::~PackageEditorState_DrawCircle() noexcept {
  Q_ASSERT(!mCurrentEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawCircle::entry() noexcept {
  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_DrawCircle::exit() noexcept {
  if (mCurrentCircle && (!abortAddCircle())) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_DrawCircle::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentCircle) {
    const Point pos = e.scenePos.mappedToGrid(getGridInterval());
    return updateCircleDiameter(pos);
  } else {
    return true;
  }
}

bool PackageEditorState_DrawCircle::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  if (mCurrentCircle) {
    return finishAddCircle(pos);
  } else {
    return startAddCircle(pos);
  }
}

bool PackageEditorState_DrawCircle::processAbortCommand() noexcept {
  if (mCurrentCircle) {
    return abortAddCircle();
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QSet<const Layer*> PackageEditorState_DrawCircle::getAvailableLayers()
    const noexcept {
  return getAllowedCircleAndPolygonLayers();
}

void PackageEditorState_DrawCircle::setLayer(const Layer& layer) noexcept {
  if (mCurrentProperties.setLayer(layer)) {
    emit layerChanged(mCurrentProperties.getLayer());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLayer(mCurrentProperties.getLayer(), true);
  }

  if (mUsedLineWidths.contains(&layer)) {
    setLineWidth(*mUsedLineWidths.find(&layer));
  } else if (layer.getPolygonsRepresentAreas()) {
    // Zero-width circles.
    setLineWidth(UnsignedLength(0));
  } else {
    // Typical width according library conventions.
    setLineWidth(UnsignedLength(200000));
  }
}

void PackageEditorState_DrawCircle::setLineWidth(
    const UnsignedLength& width) noexcept {
  if (mCurrentProperties.setLineWidth(width)) {
    emit lineWidthChanged(mCurrentProperties.getLineWidth());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLineWidth(mCurrentProperties.getLineWidth(), true);
  }

  mUsedLineWidths.insert(&mCurrentProperties.getLayer(), width);
}

void PackageEditorState_DrawCircle::setFilled(bool filled) noexcept {
  if (mCurrentProperties.setIsFilled(filled)) {
    emit filledChanged(mCurrentProperties.isFilled());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setIsFilled(mCurrentProperties.isFilled(), true);
  }
}

void PackageEditorState_DrawCircle::setGrabArea(bool grabArea) noexcept {
  if (mCurrentProperties.setIsGrabArea(grabArea)) {
    emit grabAreaChanged(mCurrentProperties.isGrabArea());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setIsGrabArea(mCurrentProperties.isGrabArea(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_DrawCircle::startAddCircle(const Point& pos) noexcept {
  if (!mContext.currentGraphicsItem) return false;

  try {
    mContext.undoStack.beginCmdGroup(tr("Add Footprint Circle"));
    mCurrentProperties.setCenter(pos);
    mCurrentCircle =
        std::make_shared<Circle>(Uuid::createRandom(), mCurrentProperties);
    mContext.undoStack.appendToCmdGroup(new CmdCircleInsert(
        mContext.currentFootprint->getCircles(), mCurrentCircle));
    mCurrentEditCmd.reset(new CmdCircleEdit(*mCurrentCircle));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentCircle);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentCircle.reset();
    mCurrentEditCmd.reset();
    return false;
  }
}

bool PackageEditorState_DrawCircle::updateCircleDiameter(
    const Point& pos) noexcept {
  Point delta = pos - mCurrentCircle->getCenter();
  Length diameter = delta.getLength() * 2;
  if (diameter < 1) {
    diameter = 1;
  }  // diameter must be greater than zero!
  mCurrentEditCmd->setDiameter(PositiveLength(diameter), true);
  return true;
}

bool PackageEditorState_DrawCircle::finishAddCircle(const Point& pos) noexcept {
  if (pos == mCurrentCircle->getCenter()) {
    return abortAddCircle();
  }

  try {
    updateCircleDiameter(pos);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentCircle.reset();
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_DrawCircle::abortAddCircle() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentCircle.reset();
    mCurrentEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
