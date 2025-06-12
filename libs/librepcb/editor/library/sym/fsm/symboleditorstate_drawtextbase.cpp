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
#include "../../../graphics/graphicsscene.h"
#include "../../../graphics/textgraphicsitem.h"
#include "../../../utils/halignactiongroup.h"
#include "../../../utils/valignactiongroup.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/layercombobox.h"
#include "../../../widgets/positivelengthedit.h"
#include "../symboleditorwidget.h"
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
    mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayer(&Layer::symbolNames()),
    mLastHeight(1),
    mLastAlignment(HAlign::left(), VAlign::bottom()),
    mLastText() {
  resetToDefaultParameters();
}

SymbolEditorState_DrawTextBase::~SymbolEditorState_DrawTextBase() noexcept {
  Q_ASSERT(!mEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_DrawTextBase::entry() noexcept {
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!startAddText(pos)) {
    return false;
  }
  mAdapter.fsmToolEnter(*this);
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
    mEditCmd->setPosition(currentPos, true);
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
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    mStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add symbol text"));
    mCurrentText =
        std::make_shared<Text>(Uuid::createRandom(), *mLastLayer, mLastText,
                               pos, mLastRotation, mLastHeight, mLastAlignment);
    mContext.undoStack.appendToCmdGroup(
        new CmdTextInsert(mContext.symbol.getTexts(), mCurrentText));
    mEditCmd.reset(new CmdTextEdit(*mCurrentText));
    mCurrentGraphicsItem = item->getGraphicsItem(mCurrentText);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
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
    mContext.undoStack.appendToCmdGroup(mEditCmd.release());
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
    mEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void SymbolEditorState_DrawTextBase::resetToDefaultParameters() noexcept {
  mLastRotation = Angle::deg0();
  switch (mMode) {
    case Mode::NAME:
      // Set all properties according library conventions
      mLastLayer = &Layer::symbolNames();
      mLastHeight = PositiveLength(2500000);
      mLastAlignment = Alignment(HAlign::left(), VAlign::bottom());
      mLastText = "{{NAME}}";
      break;
    case Mode::VALUE:
      // Set all properties according library conventions
      mLastLayer = &Layer::symbolValues();
      mLastHeight = PositiveLength(2500000);
      mLastAlignment = Alignment(HAlign::left(), VAlign::top());
      mLastText = "{{VALUE}}";
      break;
    default:
      // Set properties to something reasonable
      mLastLayer = &Layer::symbolOutlines();
      mLastHeight = PositiveLength(2500000);
      mLastAlignment = Alignment(HAlign::left(), VAlign::bottom());
      mLastText = "Text";  // Non-empty to avoid invisible graphics item
      break;
  }
}

void SymbolEditorState_DrawTextBase::layerComboBoxValueChanged(
    const Layer& layer) noexcept {
  mLastLayer = &layer;
  if (mEditCmd) {
    mEditCmd->setLayer(*mLastLayer, true);
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
