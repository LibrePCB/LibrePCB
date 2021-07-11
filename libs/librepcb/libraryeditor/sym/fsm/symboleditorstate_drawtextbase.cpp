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

#include "../symboleditorwidget.h"

#include <librepcb/common/geometry/cmd/cmdtextedit.h>
#include <librepcb/common/geometry/text.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/common/widgets/positivelengthedit.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>

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

SymbolEditorState_DrawTextBase::SymbolEditorState_DrawTextBase(
    const Context& context, Mode mode) noexcept
  : SymbolEditorState(context),
    mMode(mode),
    mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayerName(GraphicsLayer::sSymbolNames),
    mLastHeight(1) {
  resetToDefaultParameters();
}

SymbolEditorState_DrawTextBase::~SymbolEditorState_DrawTextBase() noexcept {
  Q_ASSERT(mEditCmd.isNull());
  Q_ASSERT(mCurrentText == nullptr);
  Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::entry() noexcept {
  mContext.graphicsScene.setSelectionArea(QPainterPath());  // clear selection
  mContext.graphicsView.setCursor(Qt::CrossCursor);

  // populate command toolbar
  if (mMode == Mode::TEXT) {
    mContext.commandToolBar.addLabel(tr("Layer:"));
    std::unique_ptr<GraphicsLayerComboBox> layerComboBox(
        new GraphicsLayerComboBox());
    layerComboBox->setLayers(getAllowedTextLayers());
    layerComboBox->setCurrentLayer(mLastLayerName);
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
    textComboBox->setCurrentIndex(textComboBox->findText(mLastText));
    connect(textComboBox.get(), &QComboBox::currentTextChanged, this,
            &SymbolEditorState_DrawTextBase::textComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(textComboBox));
  } else {
    resetToDefaultParameters();
  }

  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  edtHeight->configure(getDefaultLengthUnit(),
                       LengthEditBase::Steps::textHeight(),
                       "symbol_editor/draw_text/height");
  edtHeight->setValue(mLastHeight);
  connect(edtHeight.get(), &PositiveLengthEdit::valueChanged, this,
          &SymbolEditorState_DrawTextBase::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtHeight));

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  return startAddText(pos);
}

bool SymbolEditorState_DrawTextBase::exit() noexcept {
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
  return processRotateCcw();
}

bool SymbolEditorState_DrawTextBase::processRotateCw() noexcept {
  if (mCurrentText) {
    mEditCmd->rotate(-Angle::deg90(), mCurrentText->getPosition(), true);
    mLastRotation = mCurrentText->getRotation();
    return true;
  } else {
    return false;
  }
}

bool SymbolEditorState_DrawTextBase::processRotateCcw() noexcept {
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

bool SymbolEditorState_DrawTextBase::startAddText(const Point& pos) noexcept {
  try {
    mStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add symbol text"));
    mCurrentText = new Text(Uuid::createRandom(), mLastLayerName, mLastText,
                            pos, mLastRotation, mLastHeight, getAlignment());
    mContext.undoStack.appendToCmdGroup(new CmdTextInsert(
        mContext.symbol.getTexts(), std::shared_ptr<Text>(mCurrentText)));
    mEditCmd.reset(new CmdTextEdit(*mCurrentText));
    mCurrentGraphicsItem =
        mContext.symbolGraphicsItem.getTextGraphicsItem(*mCurrentText);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem = nullptr;
    mCurrentText = nullptr;
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
    mCurrentGraphicsItem = nullptr;
    mCurrentText = nullptr;
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
    mCurrentGraphicsItem = nullptr;
    mCurrentText = nullptr;
    mEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void SymbolEditorState_DrawTextBase::resetToDefaultParameters() noexcept {
  switch (mMode) {
    case Mode::NAME:
      // Set all properties according library conventions
      mLastLayerName = GraphicsLayerName(GraphicsLayer::sSymbolNames);
      mLastHeight = PositiveLength(2500000);
      mLastText = "{{NAME}}";
      break;
    case Mode::VALUE:
      // Set all properties according library conventions
      mLastLayerName = GraphicsLayerName(GraphicsLayer::sSymbolValues);
      mLastHeight = PositiveLength(2500000);
      mLastText = "{{VALUE}}";
      break;
    default:
      // Set properties to something reasonable
      mLastLayerName = GraphicsLayerName(GraphicsLayer::sSymbolOutlines);
      mLastHeight = PositiveLength(2500000);
      mLastText = "Text";  // Non-empty to avoid invisible graphics item
      break;
  }
}

Alignment SymbolEditorState_DrawTextBase::getAlignment() const noexcept {
  if (mMode == Mode::VALUE) {
    return Alignment(HAlign::left(), VAlign::top());
  } else {
    return Alignment(HAlign::left(), VAlign::bottom());
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
