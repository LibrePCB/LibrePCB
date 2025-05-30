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
#include "../../../editorcommandset.h"
#include "../../../graphics/circlegraphicsitem.h"
#include "../../../graphics/graphicslayer.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/layercombobox.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../footprintgraphicsitem.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/geometry/circle.h>
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
    mCurrentCircle(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayer(&Layer::topLegend()),  // Most important layer
    mLastLineWidth(200000),  // typical width according library conventions
    mLastFill(false),  // Fill is needed very rarely
    mLastGrabArea(false)  // Avoid creating annoying grab areas "by accident"
{
}

PackageEditorState_DrawCircle::~PackageEditorState_DrawCircle() noexcept {
  Q_ASSERT(!mEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawCircle::entry() noexcept {
  // populate command toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mContext.commandToolBar.addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(getAllowedCircleAndPolygonLayers());
  layerComboBox->setCurrentLayer(*mLastLayer);
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, this,
          &PackageEditorState_DrawCircle::layerComboBoxValueChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  mContext.commandToolBar.addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(getLengthUnit(), LengthEditBase::Steps::generic(),
                          "package_editor/draw_circle/line_width");
  edtLineWidth->setValue(mLastLineWidth);
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  connect(this, &PackageEditorState_DrawCircle::requestLineWidth,
          edtLineWidth.get(), &UnsignedLengthEdit::setValue);
  connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, this,
          &PackageEditorState_DrawCircle::lineWidthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtLineWidth));

  std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
  fillCheckBox->setChecked(mLastFill);
  fillCheckBox->addAction(cmd.fillToggle.createAction(
      fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
  connect(fillCheckBox.get(), &QCheckBox::toggled, this,
          &PackageEditorState_DrawCircle::fillCheckBoxCheckedChanged);
  mContext.commandToolBar.addWidget(std::move(fillCheckBox), 10);

  std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
  grabAreaCheckBox->setChecked(mLastGrabArea);
  grabAreaCheckBox->addAction(cmd.grabAreaToggle.createAction(
      grabAreaCheckBox.get(), grabAreaCheckBox.get(), &QCheckBox::toggle));
  connect(grabAreaCheckBox.get(), &QCheckBox::toggled, this,
          &PackageEditorState_DrawCircle::grabAreaCheckBoxCheckedChanged);
  mContext.commandToolBar.addWidget(std::move(grabAreaCheckBox));

  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_DrawCircle::exit() noexcept {
  if (mCurrentCircle && (!abortAddCircle())) {
    return false;
  }

  // cleanup command toolbar
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_DrawCircle::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
  };
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
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_DrawCircle::startAddCircle(const Point& pos) noexcept {
  try {
    mContext.undoStack.beginCmdGroup(tr("Add symbol circle"));
    mCurrentCircle = std::make_shared<Circle>(
        Uuid::createRandom(), *mLastLayer, mLastLineWidth, mLastFill,
        mLastGrabArea, pos, PositiveLength(1));
    mContext.undoStack.appendToCmdGroup(new CmdCircleInsert(
        mContext.currentFootprint->getCircles(), mCurrentCircle));
    mEditCmd.reset(new CmdCircleEdit(*mCurrentCircle));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentCircle);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentCircle.reset();
    mEditCmd.reset();
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
  mEditCmd->setDiameter(PositiveLength(diameter), true);
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
    mContext.undoStack.appendToCmdGroup(mEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_DrawCircle::abortAddCircle() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentCircle.reset();
    mEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_DrawCircle::layerComboBoxValueChanged(
    const Layer& layer) noexcept {
  mLastLayer = &layer;
  if (mEditCmd) {
    mEditCmd->setLayer(*mLastLayer, true);
  }
  if (mUsedLineWidths.contains(&layer)) {
    emit requestLineWidth(*mUsedLineWidths.find(&layer));
  } else if (layer.getPolygonsRepresentAreas()) {
    // Zero-width circles.
    emit requestLineWidth(UnsignedLength(0));
  } else {
    // Typical width according library conventions.
    emit requestLineWidth(UnsignedLength(200000));
  }
}

void PackageEditorState_DrawCircle::lineWidthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastLineWidth = value;
  if (mEditCmd) {
    mEditCmd->setLineWidth(mLastLineWidth, true);
  }
  mUsedLineWidths.insert(mLastLayer, value);
}

void PackageEditorState_DrawCircle::fillCheckBoxCheckedChanged(
    bool checked) noexcept {
  mLastFill = checked;
  if (mEditCmd) {
    mEditCmd->setIsFilled(mLastFill, true);
  }
}

void PackageEditorState_DrawCircle::grabAreaCheckBoxCheckedChanged(
    bool checked) noexcept {
  mLastGrabArea = checked;
  if (mEditCmd) {
    mEditCmd->setIsGrabArea(mLastGrabArea, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
