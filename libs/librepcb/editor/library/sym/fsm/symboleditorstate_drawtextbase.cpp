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
#include "symboleditorstate_drawtextbase.h"

#include "../../../cmd/cmdtextedit.h"
#include "../../../editorcommandset.h"
#include "../../../graphics/textgraphicsitem.h"
#include "../../../utils/halignactiongroup.h"
#include "../../../utils/valignactiongroup.h"
#include "../../../widgets/graphicslayercombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../symboleditorwidget.h"
#include "../symbolgraphicsitem.h"

#include <librepcb/core/geometry/text.h>
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/library/sym/symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorState_DrawTextBase::SymbolEditorState_DrawTextBase(
    const Context& context, Mode mode) noexcept
  : SymbolEditorState(context),
    mMode(mode),
    mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayerName(GraphicsLayer::sSymbolNames),
    mLastHeight(1),
    mLastAlignment(HAlign::left(), VAlign::bottom()),
    mLastText() {
  resetToDefaultParameters();
}

SymbolEditorState_DrawTextBase::~SymbolEditorState_DrawTextBase() noexcept {
  Q_ASSERT(mEditCmd.isNull());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::entry() noexcept {
  EditorCommandSet& cmd = EditorCommandSet::instance();

  // populate command toolbar
  if (mMode == Mode::TEXT) {
    mContext.commandToolBar.addLabel(tr("Layer:"));
    std::unique_ptr<GraphicsLayerComboBox> layerComboBox(
        new GraphicsLayerComboBox());
    layerComboBox->setLayers(getAllowedTextLayers());
    layerComboBox->setCurrentLayer(mLastLayerName);
    layerComboBox->addAction(
        cmd.layerUp.createAction(layerComboBox.get(), layerComboBox.get(),
                                 &GraphicsLayerComboBox::stepDown));
    layerComboBox->addAction(
        cmd.layerDown.createAction(layerComboBox.get(), layerComboBox.get(),
                                   &GraphicsLayerComboBox::stepUp));
    connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
            this, &SymbolEditorState_DrawTextBase::layerComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(layerComboBox));

    mContext.commandToolBar.addLabel(tr("Text:"), 10);
    std::unique_ptr<QComboBox> textComboBox(new QComboBox());
    textComboBox->setEditable(true);
    textComboBox->addItem("{{NAME}}");
    textComboBox->addItem("{{VALUE}}");
    textComboBox->addItem("{{SHEET}}");
    textComboBox->addItem("{{PROJECT}}");
    textComboBox->addItem("{{MODIFIED_DATE}}");
    textComboBox->addItem("{{AUTHOR}}");
    textComboBox->addItem("{{VERSION}}");
    textComboBox->addItem("{{PAGE_X_OF_Y}}");
    int currentTextIndex = textComboBox->findText(mLastText);
    if (currentTextIndex >= 0) {
      textComboBox->setCurrentIndex(currentTextIndex);
    } else {
      textComboBox->setCurrentText(mLastText);
    }
    connect(textComboBox.get(), &QComboBox::currentTextChanged, this,
            &SymbolEditorState_DrawTextBase::textComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(textComboBox));
  } else {
    resetToDefaultParameters();
  }

  // Height spinbox.
  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  edtHeight->configure(getLengthUnit(), LengthEditBase::Steps::textHeight(),
                       "symbol_editor/draw_text/height");
  edtHeight->setValue(mLastHeight);
  edtHeight->addAction(cmd.sizeIncrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepUp));
  edtHeight->addAction(cmd.sizeDecrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepDown));
  connect(edtHeight.get(), &PositiveLengthEdit::valueChanged, this,
          &SymbolEditorState_DrawTextBase::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtHeight));

  // Horizontal alignment
  mContext.commandToolBar.addSeparator();
  std::unique_ptr<HAlignActionGroup> hAlignActionGroup(new HAlignActionGroup());
  mHAlignActionGroup = hAlignActionGroup.get();
  hAlignActionGroup->setValue(mLastAlignment.getH());
  connect(hAlignActionGroup.get(), &HAlignActionGroup::valueChanged, this,
          &SymbolEditorState_DrawTextBase::hAlignActionGroupValueChanged);
  mContext.commandToolBar.addActionGroup(std::move(hAlignActionGroup));

  // Vertical alignment
  mContext.commandToolBar.addSeparator();
  std::unique_ptr<VAlignActionGroup> vAlignActionGroup(new VAlignActionGroup());
  mVAlignActionGroup = vAlignActionGroup.get();
  vAlignActionGroup->setValue(mLastAlignment.getV());
  connect(vAlignActionGroup.get(), &VAlignActionGroup::valueChanged, this,
          &SymbolEditorState_DrawTextBase::vAlignActionGroupValueChanged);
  mContext.commandToolBar.addActionGroup(std::move(vAlignActionGroup));

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  if (!startAddText(pos)) {
    return false;
  }
  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool SymbolEditorState_DrawTextBase::exit() noexcept {
  if (mCurrentText && !abortAddText()) {
    return false;
  }

  // cleanup command toolbar
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    SymbolEditorState_DrawTextBase::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
      EditorWidgetBase::Feature::Rotate,
      EditorWidgetBase::Feature::Mirror,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::processGraphicsSceneMouseMoved(
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

bool SymbolEditorState_DrawTextBase::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mCurrentText) {
    finishAddText(currentPos);
  }
  return startAddText(currentPos);
}

bool SymbolEditorState_DrawTextBase::
    processGraphicsSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool SymbolEditorState_DrawTextBase::processRotate(
    const Angle& rotation) noexcept {
  if (mCurrentText) {
    mEditCmd->rotate(rotation, mCurrentText->getPosition(), true);
    mLastRotation = mCurrentText->getRotation();
    return true;
  } else {
    return false;
  }
}

bool SymbolEditorState_DrawTextBase::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mCurrentText) {
    mEditCmd->mirror(orientation, mCurrentText->getPosition(), true);
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

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::startAddText(const Point& pos) noexcept {
  try {
    mStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add symbol text"));
    mCurrentText =
        std::make_shared<Text>(Uuid::createRandom(), mLastLayerName, mLastText,
                               pos, mLastRotation, mLastHeight, mLastAlignment);
    mContext.undoStack.appendToCmdGroup(
        new CmdTextInsert(mContext.symbol.getTexts(), mCurrentText));
    mEditCmd.reset(new CmdTextEdit(*mCurrentText));
    mCurrentGraphicsItem =
        mContext.symbolGraphicsItem.getGraphicsItem(mCurrentText);
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

bool SymbolEditorState_DrawTextBase::finishAddText(const Point& pos) noexcept {
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

bool SymbolEditorState_DrawTextBase::abortAddText() noexcept {
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

void SymbolEditorState_DrawTextBase::resetToDefaultParameters() noexcept {
  mLastRotation = Angle::deg0();
  switch (mMode) {
    case Mode::NAME:
      // Set all properties according library conventions
      mLastLayerName = GraphicsLayerName(GraphicsLayer::sSymbolNames);
      mLastHeight = PositiveLength(2500000);
      mLastAlignment = Alignment(HAlign::left(), VAlign::bottom());
      mLastText = "{{NAME}}";
      break;
    case Mode::VALUE:
      // Set all properties according library conventions
      mLastLayerName = GraphicsLayerName(GraphicsLayer::sSymbolValues);
      mLastHeight = PositiveLength(2500000);
      mLastAlignment = Alignment(HAlign::left(), VAlign::top());
      mLastText = "{{VALUE}}";
      break;
    default:
      // Set properties to something reasonable
      mLastLayerName = GraphicsLayerName(GraphicsLayer::sSymbolOutlines);
      mLastHeight = PositiveLength(2500000);
      mLastAlignment = Alignment(HAlign::left(), VAlign::bottom());
      mLastText = "Text";  // Non-empty to avoid invisible graphics item
      break;
  }
}

void SymbolEditorState_DrawTextBase::layerComboBoxValueChanged(
    const GraphicsLayerName& layerName) noexcept {
  mLastLayerName = layerName;
  if (mEditCmd) {
    mEditCmd->setLayerName(mLastLayerName, true);
  }
}

void SymbolEditorState_DrawTextBase::heightEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastHeight = value;
  if (mEditCmd) {
    mEditCmd->setHeight(mLastHeight, true);
  }
}

void SymbolEditorState_DrawTextBase::textComboBoxValueChanged(
    const QString& value) noexcept {
  mLastText = value.trimmed();
  if (mEditCmd) {
    mEditCmd->setText(mLastText, true);
  }
}

void SymbolEditorState_DrawTextBase::hAlignActionGroupValueChanged(
    const HAlign& value) noexcept {
  mLastAlignment.setH(value);
  if (mEditCmd) {
    mEditCmd->setAlignment(mLastAlignment, true);
  }
}

void SymbolEditorState_DrawTextBase::vAlignActionGroupValueChanged(
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
