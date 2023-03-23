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
#include "../../../editorcommandset.h"
#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/holegraphicsitem.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../footprintgraphicsitem.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/geometry/hole.h>
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
    mCurrentHole(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastDiameter(1000000)  // Commonly used drill diameter
{
}

PackageEditorState_AddHoles::~PackageEditorState_AddHoles() noexcept {
  Q_ASSERT(mEditCmd.isNull());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_AddHoles::entry() noexcept {
  // populate command toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mContext.commandToolBar.addLabel(tr("Diameter:"), 10);

  std::unique_ptr<PositiveLengthEdit> edtDiameter(new PositiveLengthEdit());
  edtDiameter->configure(getLengthUnit(),
                         LengthEditBase::Steps::drillDiameter(),
                         "package_editor/add_holes/diameter");
  edtDiameter->setValue(mLastDiameter);
  edtDiameter->addAction(cmd.drillIncrease.createAction(
      edtDiameter.get(), edtDiameter.get(), &PositiveLengthEdit::stepUp));
  edtDiameter->addAction(cmd.drillDecrease.createAction(
      edtDiameter.get(), edtDiameter.get(), &PositiveLengthEdit::stepDown));
  connect(edtDiameter.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_AddHoles::diameterEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtDiameter));

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  if (!startAddHole(pos)) {
    return false;
  }
  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_AddHoles::exit() noexcept {
  if (mCurrentHole && !abortAddHole()) {
    return false;
  }

  // cleanup command toolbar
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_AddHoles::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_AddHoles::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentHole) {
    Point currentPos =
        Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    mEditCmd->setPath(makeNonEmptyPath(currentPos), true);
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_AddHoles::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mCurrentHole) {
    finishAddHole(currentPos);
  }
  return startAddHole(currentPos);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_AddHoles::startAddHole(const Point& pos) noexcept {
  try {
    mContext.undoStack.beginCmdGroup(tr("Add hole"));
    mCurrentHole =
        std::make_shared<Hole>(Uuid::createRandom(), mLastDiameter,
                               makeNonEmptyPath(pos), MaskConfig::automatic());
    mContext.undoStack.appendToCmdGroup(
        new CmdHoleInsert(mContext.currentFootprint->getHoles(), mCurrentHole));
    mEditCmd.reset(new CmdHoleEdit(*mCurrentHole));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentHole);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentHole.reset();
    mEditCmd.reset();
    return false;
  }
}

bool PackageEditorState_AddHoles::finishAddHole(const Point& pos) noexcept {
  try {
    mEditCmd->setPath(makeNonEmptyPath(pos), true);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentHole.reset();
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_AddHoles::abortAddHole() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentHole.reset();
    mEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_AddHoles::diameterEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastDiameter = value;
  if (mEditCmd) {
    mEditCmd->setDiameter(mLastDiameter, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
