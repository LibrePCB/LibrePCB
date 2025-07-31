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
#include "packageeditorstate_addholes.h"

#include "../../../cmd/cmdholeedit.h"
#include "../../../graphics/holegraphicsitem.h"
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

PackageEditorState_AddHoles::PackageEditorState_AddHoles(
    Context& context) noexcept
  : PackageEditorState(context),
    mCurrentProperties(Uuid::createRandom(),  // Not relevant
                       PositiveLength(1000000),  // Commonly used drill diameter
                       makeNonEmptyPath(Point()),  // Not relevant
                       MaskConfig::automatic()  // Default
                       ),
    mCurrentHole(nullptr),
    mCurrentGraphicsItem(nullptr) {
}

PackageEditorState_AddHoles::~PackageEditorState_AddHoles() noexcept {
  Q_ASSERT(!mCurrentEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_AddHoles::entry() noexcept {
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!startAddHole(pos)) {
    return false;
  }

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_AddHoles::exit() noexcept {
  if (mCurrentHole && !abortAddHole()) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_AddHoles::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentHole) {
    const Point pos = e.scenePos.mappedToGrid(getGridInterval());
    mCurrentEditCmd->setPath(makeNonEmptyPath(pos), true);
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_AddHoles::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  if (mCurrentHole) {
    finishAddHole(pos);
  }
  return startAddHole(pos);
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void PackageEditorState_AddHoles::setDiameter(
    const PositiveLength& diameter) noexcept {
  if (mCurrentProperties.setDiameter(diameter)) {
    emit diameterChanged(mCurrentProperties.getDiameter());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setDiameter(mCurrentProperties.getDiameter(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_AddHoles::startAddHole(const Point& pos) noexcept {
  if (!mContext.currentGraphicsItem) return false;

  try {
    mContext.undoStack.beginCmdGroup(tr("Add Footprint Hole"));
    mCurrentProperties.setPath(makeNonEmptyPath(pos));
    mCurrentHole =
        std::make_shared<Hole>(Uuid::createRandom(), mCurrentProperties);
    mContext.undoStack.appendToCmdGroup(
        new CmdHoleInsert(mContext.currentFootprint->getHoles(), mCurrentHole));
    mCurrentEditCmd.reset(new CmdHoleEdit(*mCurrentHole));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentHole);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentHole.reset();
    mCurrentEditCmd.reset();
    return false;
  }
}

bool PackageEditorState_AddHoles::finishAddHole(const Point& pos) noexcept {
  try {
    mCurrentEditCmd->setPath(makeNonEmptyPath(pos), true);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentHole.reset();
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_AddHoles::abortAddHole() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentHole.reset();
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
