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
#include "packageeditorstate_drawtextbase.h"

#include "../packageeditorwidget.h"

#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/common/widgets/halignactiongroup.h>
#include <librepcb/common/widgets/positivelengthedit.h>
#include <librepcb/common/widgets/unsignedlengthedit.h>
#include <librepcb/common/widgets/valignactiongroup.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_DrawTextBase::PackageEditorState_DrawTextBase(
    Context& context, Mode mode) noexcept
  : PackageEditorState(context),
    mMode(mode),
    mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayerName(GraphicsLayer::sTopNames),
    mLastHeight(1),
    mLastStrokeWidth(0),
    mLastAlignment(HAlign::left(), VAlign::bottom()),
    mLastText() {
  resetToDefaultParameters();
}

PackageEditorState_DrawTextBase::~PackageEditorState_DrawTextBase() noexcept {
  Q_ASSERT(mEditCmd.isNull());
  Q_ASSERT(mCurrentText == nullptr);
  Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawTextBase::entry() noexcept {
  mContext.graphicsScene.setSelectionArea(QPainterPath());  // clear selection
  mContext.graphicsView.setCursor(Qt::CrossCursor);

  // populate command toolbar
  if (mMode == Mode::TEXT) {
    mContext.commandToolBar.addLabel(tr("Layer:"));
    std::unique_ptr<GraphicsLayerComboBox> layerComboBox(
        new GraphicsLayerComboBox());
    layerComboBox->setLayers(
        mContext.layerProvider.getBoardGeometryElementLayers());
    layerComboBox->setCurrentLayer(mLastLayerName);
    connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
            this, &PackageEditorState_DrawTextBase::layerComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(layerComboBox));

    mContext.commandToolBar.addLabel(tr("Text:"), 10);
    std::unique_ptr<QComboBox> textComboBox(new QComboBox());
    textComboBox->setEditable(true);
    textComboBox->addItem("{{NAME}}");
    textComboBox->addItem("{{VALUE}}");
    int currentTextIndex = textComboBox->findText(mLastText);
    if (currentTextIndex >= 0) {
      textComboBox->setCurrentIndex(currentTextIndex);
    } else {
      textComboBox->setCurrentText(mLastText);
    }
    connect(textComboBox.get(), &QComboBox::currentTextChanged, this,
            &PackageEditorState_DrawTextBase::textComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(textComboBox));
  } else {
    resetToDefaultParameters();
  }

  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtLineWidth(new PositiveLengthEdit());
  edtLineWidth->setSingleStep(0.5);  // [mm]
  edtLineWidth->setValue(mLastHeight);
  connect(edtLineWidth.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_DrawTextBase::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtLineWidth));

  // Stroke width
  mContext.commandToolBar.addLabel(tr("Stroke Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> strokeWidthSpinBox(
      new UnsignedLengthEdit());
  strokeWidthSpinBox->setSingleStep(0.1);  // [mm]
  strokeWidthSpinBox->setValue(mLastStrokeWidth);
  connect(strokeWidthSpinBox.get(), &UnsignedLengthEdit::valueChanged, this,
          &PackageEditorState_DrawTextBase::strokeWidthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(strokeWidthSpinBox));

  // Horizontal alignment
  mContext.commandToolBar.addSeparator();
  std::unique_ptr<HAlignActionGroup> hAlignActionGroup(new HAlignActionGroup());
  hAlignActionGroup->setValue(mLastAlignment.getH());
  connect(hAlignActionGroup.get(), &HAlignActionGroup::valueChanged, this,
          &PackageEditorState_DrawTextBase::hAlignActionGroupValueChanged);
  mContext.commandToolBar.addActionGroup(std::move(hAlignActionGroup));

  // Vertical alignment
  mContext.commandToolBar.addSeparator();
  std::unique_ptr<VAlignActionGroup> vAlignActionGroup(new VAlignActionGroup());
  vAlignActionGroup->setValue(mLastAlignment.getV());
  connect(vAlignActionGroup.get(), &VAlignActionGroup::valueChanged, this,
          &PackageEditorState_DrawTextBase::vAlignActionGroupValueChanged);
  mContext.commandToolBar.addActionGroup(std::move(vAlignActionGroup));

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  return startAddText(pos);
}

bool PackageEditorState_DrawTextBase::exit() noexcept {
  if (mCurrentText && !abortAddText()) {
    return false;
  }

  // cleanup command toolbar
  mContext.commandToolBar.clear();

  mContext.graphicsView.setCursor(Qt::ArrowCursor);
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_DrawTextBase::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentText) {
    Point currentPos =
        Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    mEditCmd->setPosition(currentPos, true);
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_DrawTextBase::
    processGraphicsSceneLeftMouseButtonPressed(
        QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mCurrentText) {
    finishAddText(currentPos);
  }
  return startAddText(currentPos);
}

bool PackageEditorState_DrawTextBase::
    processGraphicsSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotateCcw();
}
bool PackageEditorState_DrawTextBase::processRotateCw() noexcept {
  if (mCurrentText) {
    mEditCmd->rotate(-Angle::deg90(), mCurrentText->getPosition(), true);
    mLastRotation = mCurrentText->getRotation();
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_DrawTextBase::processRotateCcw() noexcept {
  if (mCurrentText) {
    mEditCmd->rotate(Angle::deg90(), mCurrentText->getPosition(), true);
    mLastRotation = mCurrentText->getRotation();
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_DrawTextBase::startAddText(const Point& pos) noexcept {
  try {
    mStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add footprint text"));
    mCurrentText = new StrokeText(
        Uuid::createRandom(), mLastLayerName, mLastText, pos, mLastRotation,
        mLastHeight, mLastStrokeWidth, StrokeTextSpacing(), StrokeTextSpacing(),
        mLastAlignment, false, true);
    mContext.undoStack.appendToCmdGroup(
        new CmdStrokeTextInsert(mContext.currentFootprint->getStrokeTexts(),
                                std::shared_ptr<StrokeText>(mCurrentText)));
    mEditCmd.reset(new CmdStrokeTextEdit(*mCurrentText));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getTextGraphicsItem(*mCurrentText);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem = nullptr;
    mCurrentText         = nullptr;
    mEditCmd.reset();
    return false;
  }
}

bool PackageEditorState_DrawTextBase::finishAddText(const Point& pos) noexcept {
  if (pos == mStartPos) {
    return abortAddText();
  }

  try {
    mEditCmd->setPosition(pos, true);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem = nullptr;
    mCurrentText         = nullptr;
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_DrawTextBase::abortAddText() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem = nullptr;
    mCurrentText         = nullptr;
    mEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_DrawTextBase::resetToDefaultParameters() noexcept {
  switch (mMode) {
    case Mode::NAME:
      // Set all properties according library conventions
      mLastLayerName   = GraphicsLayerName(GraphicsLayer::sTopNames);
      mLastHeight      = PositiveLength(1000000);
      mLastStrokeWidth = UnsignedLength(200000);
      mLastAlignment   = Alignment(HAlign::center(), VAlign::bottom());
      mLastText        = "{{NAME}}";
      break;
    case Mode::VALUE:
      // Set all properties according library conventions
      mLastLayerName   = GraphicsLayerName(GraphicsLayer::sTopValues);
      mLastHeight      = PositiveLength(1000000);
      mLastStrokeWidth = UnsignedLength(200000);
      mLastAlignment   = Alignment(HAlign::center(), VAlign::top());
      mLastText        = "{{VALUE}}";
      break;
    default:
      // Set properties to something reasonable
      mLastLayerName   = GraphicsLayerName(GraphicsLayer::sTopPlacement);
      mLastHeight      = PositiveLength(2000000);
      mLastStrokeWidth = UnsignedLength(200000);
      mLastAlignment   = Alignment(HAlign::left(), VAlign::bottom());
      mLastText        = "Text";  // Non-empty to avoid invisible graphics item
      break;
  }
}

void PackageEditorState_DrawTextBase::layerComboBoxValueChanged(
    const GraphicsLayerName& layerName) noexcept {
  mLastLayerName = layerName;
  if (mEditCmd) {
    mEditCmd->setLayerName(mLastLayerName, true);
  }
}

void PackageEditorState_DrawTextBase::heightEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastHeight = value;
  if (mEditCmd) {
    mEditCmd->setHeight(mLastHeight, true);
  }
}

void PackageEditorState_DrawTextBase::strokeWidthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastStrokeWidth = value;
  if (mEditCmd) {
    mEditCmd->setStrokeWidth(mLastStrokeWidth, true);
  }
}

void PackageEditorState_DrawTextBase::textComboBoxValueChanged(
    const QString& value) noexcept {
  mLastText = value.trimmed();
  if (mEditCmd) {
    mEditCmd->setText(mLastText, true);
  }
}

void PackageEditorState_DrawTextBase::hAlignActionGroupValueChanged(
    const HAlign& value) noexcept {
  mLastAlignment.setH(value);
  if (mEditCmd) {
    mEditCmd->setAlignment(mLastAlignment, true);
  }
}

void PackageEditorState_DrawTextBase::vAlignActionGroupValueChanged(
    const VAlign& value) noexcept {
  mLastAlignment.setV(value);
  if (mEditCmd) {
    mEditCmd->setAlignment(mLastAlignment, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
