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

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static Text getInitialProperties(SymbolEditorState_DrawTextBase::Mode mode,
                                 const ClientSettings& settings,
                                 const QSet<const Layer*>& layers) noexcept {
  switch (mode) {
    case SymbolEditorState_DrawTextBase::Mode::NAME:
      // Set all properties according library conventions
      return Text{
          Uuid::createRandom(),  // Not relevant
          Layer::symbolNames(),  // Layer
          "{{NAME}}",  // Text
          Point(),  // Position
          Angle::deg0(),  // Rotation
          SymbolEditorState_DrawTextBase::getDefaultHeight(),  // Height
          Alignment(HAlign::left(), VAlign::bottom()),  // Alignment
          false  // Locked
      };
    case SymbolEditorState_DrawTextBase::Mode::VALUE:
      // Set all properties according library conventions
      return Text{
          Uuid::createRandom(),  // Not relevant
          Layer::symbolValues(),  // Layer
          "{{VALUE}}",  // Text
          Point(),  // Position
          Angle::deg0(),  // Rotation
          SymbolEditorState_DrawTextBase::getDefaultHeight(),  // Height
          Alignment(HAlign::left(), VAlign::top()),  // Alignment
          false  // Locked
      };
    default:
      return Text{
          Uuid::createRandom(),  // Not relevant
          *settings.get("layer", layers)
               .value_or(&Layer::symbolOutlines()),  // Layer
          "Text",  // Non-empty to avoid invisible graphics item
          Point(),  // Position
          Angle::deg0(),  // Rotation
          SymbolEditorState_DrawTextBase::getDefaultHeight(),  // Height
          Alignment(HAlign::left(), VAlign::bottom()),  // Alignment
          true  // Locked
      };
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorState_DrawTextBase::SymbolEditorState_DrawTextBase(
    const Context& context, QAnyStringView settingsGroup, Mode mode) noexcept
  : SymbolEditorState(context, settingsGroup),
    mMode(mode),
    mCurrentProperties(
        getInitialProperties(mode, mSettings, getAvailableLayers())),
    mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr) {
  loadLayerSpecificClientSettings();
}

SymbolEditorState_DrawTextBase::~SymbolEditorState_DrawTextBase() noexcept {
  Q_ASSERT(!mCurrentEditCmd);
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
      emit alignmentChanged(mCurrentProperties.getAlign());
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
  if (!mCurrentProperties.setLayer(layer)) {
    return;
  }

  mSettings.set("layer", mCurrentProperties.getLayer());
  emit layerChanged(mCurrentProperties.getLayer());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLayer(mCurrentProperties.getLayer(), true);
  }

  loadLayerSpecificClientSettings();
}

void SymbolEditorState_DrawTextBase::setText(const QString& text) noexcept {
  if (!mCurrentProperties.setText(text)) {
    return;
  }

  emit textChanged(mCurrentProperties.getText());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setText(mCurrentProperties.getText(), true);
  }
}

const QStringList SymbolEditorState_DrawTextBase::getTextSuggestions()
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

PositiveLength SymbolEditorState_DrawTextBase::getDefaultHeight() noexcept {
  // According library conventions - do not change if the conventions have
  // not changed!
  return PositiveLength(2500000);
}

void SymbolEditorState_DrawTextBase::setHeight(
    const PositiveLength& height) noexcept {
  if (!mCurrentProperties.setHeight(height)) {
    return;
  }

  mSettings.set("height/by_layer/" % mCurrentProperties.getLayer().getId(),
                mCurrentProperties.getHeight());
  emit heightChanged(mCurrentProperties.getHeight());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setHeight(mCurrentProperties.getHeight(), true);
  }
}

void SymbolEditorState_DrawTextBase::resetHeight() noexcept {
  setHeight(getDefaultHeight());
}

void SymbolEditorState_DrawTextBase::setAlign(const Alignment& align) noexcept {
  if (!mCurrentProperties.setAlign(align)) {
    return;
  }

  emit alignmentChanged(mCurrentProperties.getAlign());

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
    mCurrentEditCmd = std::make_unique<CmdTextEdit>(*mCurrentText);
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

void SymbolEditorState_DrawTextBase::
    loadLayerSpecificClientSettings() noexcept {
  if (mMode == Mode::TEXT) {
    if (auto v = mSettings.get<PositiveLength>(
            "height/by_layer/" % mCurrentProperties.getLayer().getId())) {
      setHeight(*v);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
