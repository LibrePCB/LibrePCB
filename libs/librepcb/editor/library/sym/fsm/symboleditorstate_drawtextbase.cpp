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
#include "../../../graphics/textgraphicsitem.h"
#include "../../../undostack.h"
#include "../symbolgraphicsitem.h"

#include <librepcb/core/geometry/text.h>
#include <librepcb/core/library/sym/symbol.h>
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

SymbolEditorState_DrawTextBase::SymbolEditorState_DrawTextBase(
    const Context& context, Mode mode) noexcept
  : SymbolEditorState(context),
    mMode(mode),
    mCurrentProperties(Uuid::createRandom(),  // Not relevant
                       Layer::symbolNames(),  // Layer
                       QString(),  // Text
                       Point(),  // Position
                       Angle::deg0(),  // Rotation
                       PositiveLength(1),  // Height
                       Alignment(HAlign::left(), VAlign::bottom())  // Alignment
                       ),
    mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr) {
  resetToDefaultParameters();
}

SymbolEditorState_DrawTextBase::~SymbolEditorState_DrawTextBase() noexcept {
  Q_ASSERT(!mCurrentEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::entry() noexcept {
  if (mMode != Mode::TEXT) {
    resetToDefaultParameters();
  }

  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!startAddText(pos)) {
    return false;
  }
  notifyToolEnter();
  mAdapter.fsmSetFeatures(SymbolEditorFsmAdapter::Feature::Rotate |
                          SymbolEditorFsmAdapter::Feature::Mirror);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SymbolEditorState_DrawTextBase::exit() noexcept {
  if (mCurrentText && !abortAddText()) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(SymbolEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentText) {
    Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
    mCurrentEditCmd->setPosition(currentPos, true);
    return true;
  } else {
    return false;
  }
}

bool SymbolEditorState_DrawTextBase::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
  if (mCurrentText) {
    finishAddText(currentPos);
  }
  return startAddText(currentPos);
}

bool SymbolEditorState_DrawTextBase::
    processGraphicsSceneRightMouseButtonReleased(
        const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool SymbolEditorState_DrawTextBase::processRotate(
    const Angle& rotation) noexcept {
  if (mCurrentText) {
    mCurrentEditCmd->rotate(rotation, mCurrentText->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentText->getRotation());
    return true;
  } else {
    return false;
  }
}

bool SymbolEditorState_DrawTextBase::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mCurrentText) {
    mCurrentEditCmd->mirror(orientation, mCurrentText->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentText->getRotation());
    if (mCurrentProperties.setAlign(mCurrentText->getAlign())) {
      emit hAlignChanged(mCurrentProperties.getAlign().getH());
      emit vAlignChanged(mCurrentProperties.getAlign().getV());
    }
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QSet<const Layer*> SymbolEditorState_DrawTextBase::getAvailableLayers()
    const noexcept {
  return getAllowedTextLayers();
}

void SymbolEditorState_DrawTextBase::setLayer(const Layer& layer) noexcept {
  if (mCurrentProperties.setLayer(layer)) {
    emit layerChanged(mCurrentProperties.getLayer());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLayer(mCurrentProperties.getLayer(), true);
  }
}

void SymbolEditorState_DrawTextBase::setText(const QString& text) noexcept {
  if (mCurrentProperties.setText(text)) {
    emit textChanged(mCurrentProperties.getText());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setText(mCurrentProperties.getText(), true);
  }
}

QStringList SymbolEditorState_DrawTextBase::getTextSuggestions()
    const noexcept {
  if (mMode == Mode::TEXT) {
    return {
        "{{NAME}}",  //
        "{{VALUE}}",  //
        "{{SHEET}}",  //
        "{{PROJECT}}",  //
        "{{DATE}}",  //
        "{{TIME}}",  //
        "{{AUTHOR}}",  //
        "{{VERSION}}",  //
        "{{PAGE_X_OF_Y}}",  //
    };
  } else {
    return QStringList();
  }
}

void SymbolEditorState_DrawTextBase::setHeight(
    const PositiveLength& height) noexcept {
  if (mCurrentProperties.setHeight(height)) {
    emit heightChanged(mCurrentProperties.getHeight());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setHeight(mCurrentProperties.getHeight(), true);
  }
}

void SymbolEditorState_DrawTextBase::setHAlign(const HAlign& align) noexcept {
  if (mCurrentProperties.setAlign(
          Alignment(align, mCurrentProperties.getAlign().getV()))) {
    emit hAlignChanged(mCurrentProperties.getAlign().getH());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setAlignment(mCurrentProperties.getAlign(), true);
  }
}

void SymbolEditorState_DrawTextBase::setVAlign(const VAlign& align) noexcept {
  if (mCurrentProperties.setAlign(
          Alignment(mCurrentProperties.getAlign().getH(), align))) {
    emit vAlignChanged(mCurrentProperties.getAlign().getV());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setAlignment(mCurrentProperties.getAlign(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::startAddText(const Point& pos) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    mStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add symbol text"));
    mCurrentProperties.setPosition(pos);
    mCurrentText =
        std::make_shared<Text>(Uuid::createRandom(), mCurrentProperties);
    mContext.undoStack.appendToCmdGroup(
        new CmdTextInsert(mContext.symbol.getTexts(), mCurrentText));
    mCurrentEditCmd.reset(new CmdTextEdit(*mCurrentText));
    mCurrentGraphicsItem = item->getGraphicsItem(mCurrentText);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentText.reset();
    mCurrentEditCmd.reset();
    return false;
  }
}

bool SymbolEditorState_DrawTextBase::finishAddText(const Point& pos) noexcept {
  if (pos == mStartPos) {
    return abortAddText();
  }

  try {
    mCurrentEditCmd->setPosition(pos, true);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentText.reset();
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SymbolEditorState_DrawTextBase::abortAddText() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentText.reset();
    mCurrentEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void SymbolEditorState_DrawTextBase::resetToDefaultParameters() noexcept {
  mCurrentProperties.setRotation(Angle::deg0());
  switch (mMode) {
    case Mode::NAME:
      // Set all properties according library conventions
      setLayer(Layer::symbolNames());
      setHeight(PositiveLength(2500000));
      setHAlign(HAlign::left());
      setVAlign(VAlign::bottom());
      setText("{{NAME}}");
      break;
    case Mode::VALUE:
      // Set all properties according library conventions
      setLayer(Layer::symbolValues());
      setHeight(PositiveLength(2500000));
      setHAlign(HAlign::left());
      setVAlign(VAlign::top());
      setText("{{VALUE}}");
      break;
    default:
      // Set properties to something reasonable
      setLayer(Layer::symbolOutlines());
      setHeight(PositiveLength(2500000));
      setHAlign(HAlign::left());
      setVAlign(VAlign::bottom());
      setText("Text");  // Non-empty to avoid invisible graphics item
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
