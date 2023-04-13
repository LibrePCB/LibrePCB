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

#include "../../../cmd/cmdstroketextedit.h"
#include "../../../editorcommandset.h"
#include "../../../graphics/stroketextgraphicsitem.h"
#include "../../../utils/halignactiongroup.h"
#include "../../../utils/valignactiongroup.h"
#include "../../../widgets/graphicslayercombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../footprintgraphicsitem.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
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
    mLastLayer(&Layer::topNames()),
    mLastHeight(1),
    mLastStrokeWidth(0),
    mLastAlignment(HAlign::left(), VAlign::bottom()),
    mLastText(),
    mLastMirrored(false) {
  resetToDefaultParameters();
}

PackageEditorState_DrawTextBase::~PackageEditorState_DrawTextBase() noexcept {
  Q_ASSERT(mEditCmd.isNull());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawTextBase::entry() noexcept {
  // populate command toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();
  if (mMode == Mode::TEXT) {
    mContext.commandToolBar.addLabel(tr("Layer:"));
    std::unique_ptr<GraphicsLayerComboBox> layerComboBox(
        new GraphicsLayerComboBox());
    mLayerComboBox = layerComboBox.get();
    layerComboBox->setLayers(getAllowedTextLayers());
    layerComboBox->setCurrentLayer(*mLastLayer);
    layerComboBox->addAction(
        cmd.layerUp.createAction(layerComboBox.get(), layerComboBox.get(),
                                 &GraphicsLayerComboBox::stepDown));
    layerComboBox->addAction(
        cmd.layerDown.createAction(layerComboBox.get(), layerComboBox.get(),
                                   &GraphicsLayerComboBox::stepUp));
    connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
            this, &PackageEditorState_DrawTextBase::layerComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(layerComboBox));

    mContext.commandToolBar.addLabel(tr("Text:"), 10);
    std::unique_ptr<QComboBox> textComboBox(new QComboBox());
    textComboBox->setEditable(true);
    textComboBox->addItem("{{NAME}}");
    textComboBox->addItem("{{VALUE}}");
    textComboBox->addItem("{{BOARD}}");
    textComboBox->addItem("{{PROJECT}}");
    textComboBox->addItem("{{AUTHOR}}");
    textComboBox->addItem("{{VERSION}}");
    textComboBox->addItem("{{DATE}}");
    textComboBox->addItem("{{TIME}}");
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

  // Height.
  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  edtHeight->configure(getLengthUnit(), LengthEditBase::Steps::textHeight(),
                       "package_editor/draw_text/height");
  edtHeight->setValue(mLastHeight);
  edtHeight->addAction(cmd.sizeIncrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepUp));
  edtHeight->addAction(cmd.sizeDecrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepDown));
  connect(edtHeight.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_DrawTextBase::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtHeight));

  // Stroke width
  mContext.commandToolBar.addLabel(tr("Stroke Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> strokeWidthSpinBox(
      new UnsignedLengthEdit());
  strokeWidthSpinBox->configure(getLengthUnit(),
                                LengthEditBase::Steps::generic(),
                                "package_editor/draw_text/stroke_width");
  strokeWidthSpinBox->setValue(mLastStrokeWidth);
  strokeWidthSpinBox->addAction(cmd.lineWidthIncrease.createAction(
      strokeWidthSpinBox.get(), strokeWidthSpinBox.get(),
      &UnsignedLengthEdit::stepUp));
  strokeWidthSpinBox->addAction(cmd.lineWidthDecrease.createAction(
      strokeWidthSpinBox.get(), strokeWidthSpinBox.get(),
      &UnsignedLengthEdit::stepDown));
  connect(strokeWidthSpinBox.get(), &UnsignedLengthEdit::valueChanged, this,
          &PackageEditorState_DrawTextBase::strokeWidthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(strokeWidthSpinBox));

  // Horizontal alignment
  mContext.commandToolBar.addSeparator();
  std::unique_ptr<HAlignActionGroup> hAlignActionGroup(new HAlignActionGroup());
  mHAlignActionGroup = hAlignActionGroup.get();
  hAlignActionGroup->setValue(mLastAlignment.getH());
  connect(hAlignActionGroup.get(), &HAlignActionGroup::valueChanged, this,
          &PackageEditorState_DrawTextBase::hAlignActionGroupValueChanged);
  mContext.commandToolBar.addActionGroup(std::move(hAlignActionGroup));

  // Vertical alignment
  mContext.commandToolBar.addSeparator();
  std::unique_ptr<VAlignActionGroup> vAlignActionGroup(new VAlignActionGroup());
  mVAlignActionGroup = vAlignActionGroup.get();
  vAlignActionGroup->setValue(mLastAlignment.getV());
  connect(vAlignActionGroup.get(), &VAlignActionGroup::valueChanged, this,
          &PackageEditorState_DrawTextBase::vAlignActionGroupValueChanged);
  mContext.commandToolBar.addActionGroup(std::move(vAlignActionGroup));

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  if (!startAddText(pos)) {
    return false;
  }
  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_DrawTextBase::exit() noexcept {
  if (mCurrentText && !abortAddText()) {
    return false;
  }

  // cleanup command toolbar
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_DrawTextBase::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
      EditorWidgetBase::Feature::Rotate,
      EditorWidgetBase::Feature::Mirror,
      EditorWidgetBase::Feature::Flip,
  };
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
  return processRotate(Angle::deg90());
}

bool PackageEditorState_DrawTextBase::processRotate(
    const Angle& rotation) noexcept {
  if (mCurrentText) {
    mEditCmd->rotate(rotation, mCurrentText->getPosition(), true);
    mLastRotation = mCurrentText->getRotation();
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_DrawTextBase::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mCurrentText) {
    mEditCmd->mirrorGeometry(orientation, mCurrentText->getPosition(), true);
    mLastRotation = mCurrentText->getRotation();
    mLastAlignment = mCurrentText->getAlign();
    if (mHAlignActionGroup) {
      mHAlignActionGroup->setValue(mLastAlignment.getH());
    }
    if (mHAlignActionGroup) {
      mVAlignActionGroup->setValue(mLastAlignment.getV());
    }
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_DrawTextBase::processFlip(
    Qt::Orientation orientation) noexcept {
  if (mCurrentText) {
    mEditCmd->mirrorGeometry(orientation, mCurrentText->getPosition(), true);
    mEditCmd->mirrorLayer(true);
    mLastLayer = &mCurrentText->getLayer();
    mLastRotation = mCurrentText->getRotation();
    mLastAlignment = mCurrentText->getAlign();
    mLastMirrored = mCurrentText->getMirrored();
    if (mLayerComboBox) {
      mLayerComboBox->setCurrentLayer(mCurrentText->getLayer());
    }
    if (mHAlignActionGroup) {
      mHAlignActionGroup->setValue(mLastAlignment.getH());
    }
    if (mHAlignActionGroup) {
      mVAlignActionGroup->setValue(mLastAlignment.getV());
    }
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
    mCurrentText = std::make_shared<StrokeText>(
        Uuid::createRandom(), *mLastLayer, mLastText, pos, mLastRotation,
        mLastHeight, mLastStrokeWidth, StrokeTextSpacing(), StrokeTextSpacing(),
        mLastAlignment, mLastMirrored, true);
    mContext.undoStack.appendToCmdGroup(new CmdStrokeTextInsert(
        mContext.currentFootprint->getStrokeTexts(), mCurrentText));
    mEditCmd.reset(new CmdStrokeTextEdit(*mCurrentText));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentText);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentText.reset();
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
    mCurrentGraphicsItem.reset();
    mCurrentText.reset();
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
    mCurrentGraphicsItem.reset();
    mCurrentText.reset();
    mEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_DrawTextBase::resetToDefaultParameters() noexcept {
  mLastRotation = Angle::deg0();
  mLastMirrored = false;
  switch (mMode) {
    case Mode::NAME:
      // Set all properties according library conventions
      mLastLayer = &Layer::topNames();
      mLastHeight = PositiveLength(1000000);
      mLastStrokeWidth = UnsignedLength(200000);
      mLastAlignment = Alignment(HAlign::center(), VAlign::bottom());
      mLastText = "{{NAME}}";
      break;
    case Mode::VALUE:
      // Set all properties according library conventions
      mLastLayer = &Layer::topValues();
      mLastHeight = PositiveLength(1000000);
      mLastStrokeWidth = UnsignedLength(200000);
      mLastAlignment = Alignment(HAlign::center(), VAlign::top());
      mLastText = "{{VALUE}}";
      break;
    default:
      // Set properties to something reasonable
      mLastLayer = &Layer::topPlacement();
      mLastHeight = PositiveLength(2000000);
      mLastStrokeWidth = UnsignedLength(200000);
      mLastAlignment = Alignment(HAlign::left(), VAlign::bottom());
      mLastText = "Text";  // Non-empty to avoid invisible graphics item
      break;
  }
}

void PackageEditorState_DrawTextBase::layerComboBoxValueChanged(
    const Layer& layer) noexcept {
  mLastLayer = &layer;
  if (mEditCmd) {
    mEditCmd->setLayer(*mLastLayer, true);
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
}  // namespace librepcb
