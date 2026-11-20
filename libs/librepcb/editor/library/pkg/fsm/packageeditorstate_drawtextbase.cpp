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
#include "../../../graphics/stroketextgraphicsitem.h"
#include "../../../undostack.h"
#include "../footprintgraphicsitem.h"

#include <librepcb/core/library/pkg/footprint.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static StrokeText getInitialProperties(
    PackageEditorState_DrawTextBase::Mode mode, const ClientSettings& settings,
    const QSet<const Layer*>& layers) noexcept {
  switch (mode) {
    case PackageEditorState_DrawTextBase::Mode::NAME:
      // Set all properties according library conventions
      return StrokeText{
          Uuid::createRandom(),  // Not relevant
          Layer::topNames(),  // Layer
          "{{NAME}}",  // Text
          Point(),  // Position
          Angle::deg0(),  // Rotation
          PackageEditorState_DrawTextBase::getDefaultHeight(Layer::topNames()),
          PackageEditorState_DrawTextBase::getDefaultStrokeWidth(),  // Stroke
          StrokeTextSpacing(),  // Letter spacing
          StrokeTextSpacing(),  // Line spacing
          Alignment(HAlign::center(), VAlign::bottom()),  // Alignment
          false,  // Mirror
          true,  // Auto rotate
          false  // Locked
      };
    case PackageEditorState_DrawTextBase::Mode::VALUE:
      // Set all properties according library conventions
      return StrokeText{
          Uuid::createRandom(),  // Not relevant
          Layer::topValues(),  // Layer
          "{{VALUE}}",  // Text
          Point(),  // Position
          Angle::deg0(),  // Rotation
          PackageEditorState_DrawTextBase::getDefaultHeight(Layer::topValues()),
          PackageEditorState_DrawTextBase::getDefaultStrokeWidth(),  // Stroke
          StrokeTextSpacing(),  // Letter spacing
          StrokeTextSpacing(),  // Line spacing
          Alignment(HAlign::center(), VAlign::top()),  // Alignment
          false,  // Mirror
          true,  // Auto rotate
          false  // Locked
      };
    default:
      const Layer& layer =
          *settings.get("layer", layers).value_or(&Layer::topLegend());
      return StrokeText{
          Uuid::createRandom(),  // Not relevant
          layer,  // Layer
          "Text",  // Non-empty to avoid invisible graphics item
          Point(),  // Position
          Angle::deg0(),  // Rotation
          PackageEditorState_DrawTextBase::getDefaultHeight(layer),  // Height
          PackageEditorState_DrawTextBase::getDefaultStrokeWidth(),  // Stroke
          StrokeTextSpacing(),  // Letter spacing
          StrokeTextSpacing(),  // Line spacing
          Alignment(HAlign::left(), VAlign::bottom()),  // Alignment
          false,  // Mirror
          true,  // Auto rotate
          true  // Locked
      };
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_DrawTextBase::PackageEditorState_DrawTextBase(
    Context& context, QAnyStringView settingsGroup, Mode mode) noexcept
  : PackageEditorState(context, settingsGroup),
    mMode(mode),
    mCurrentProperties(
        getInitialProperties(mode, mSettings, getAvailableLayers())),
    mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr) {
  loadLayerSpecificClientSettings();
}

PackageEditorState_DrawTextBase::~PackageEditorState_DrawTextBase() noexcept {
  Q_ASSERT(!mCurrentEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawTextBase::entry() noexcept {
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!startAddText(pos)) {
    return false;
  }
  notifyToolEnter();
  mAdapter.fsmSetFeatures(PackageEditorFsmAdapter::Feature::Rotate |
                          PackageEditorFsmAdapter::Feature::Mirror |
                          PackageEditorFsmAdapter::Feature::Flip);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_DrawTextBase::exit() noexcept {
  if (mCurrentText && !abortAddText()) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(PackageEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_DrawTextBase::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentText) {
    Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
    mCurrentEditCmd->setPosition(currentPos, true);
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_DrawTextBase::
    processGraphicsSceneLeftMouseButtonPressed(
        const GraphicsSceneMouseEvent& e) noexcept {
  Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
  if (mCurrentText) {
    finishAddText(currentPos);
  }
  return startAddText(currentPos);
}

bool PackageEditorState_DrawTextBase::
    processGraphicsSceneRightMouseButtonReleased(
        const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool PackageEditorState_DrawTextBase::processRotate(
    const Angle& rotation) noexcept {
  if (mCurrentText) {
    mCurrentEditCmd->rotate(rotation, mCurrentText->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentText->getRotation());
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_DrawTextBase::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mCurrentText) {
    mCurrentEditCmd->mirrorGeometry(orientation, mCurrentText->getPosition(),
                                    true);
    mCurrentProperties.setRotation(mCurrentText->getRotation());
    if (mCurrentProperties.setAlign(mCurrentText->getAlign())) {
      emit alignmentChanged(mCurrentProperties.getAlign());
    }
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_DrawTextBase::processFlip(
    Qt::Orientation orientation) noexcept {
  if (mCurrentText) {
    mCurrentEditCmd->mirrorGeometry(orientation, mCurrentText->getPosition(),
                                    true);
    mCurrentEditCmd->mirrorLayer(true);
    if (mCurrentProperties.setLayer(mCurrentText->getLayer())) {
      emit layerChanged(mCurrentProperties.getLayer());
    }
    mCurrentProperties.setRotation(mCurrentText->getRotation());
    if (mCurrentProperties.setAlign(mCurrentText->getAlign())) {
      emit alignmentChanged(mCurrentProperties.getAlign());
    }
    mCurrentProperties.setMirrored(mCurrentText->getMirrored());
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QSet<const Layer*> PackageEditorState_DrawTextBase::getAvailableLayers()
    const noexcept {
  return getAllowedTextLayers();
}

void PackageEditorState_DrawTextBase::setLayer(const Layer& layer) noexcept {
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

void PackageEditorState_DrawTextBase::setText(const QString& text) noexcept {
  if (!mCurrentProperties.setText(text)) {
    return;
  }

  emit textChanged(mCurrentProperties.getText());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setText(mCurrentProperties.getText(), true);
  }
}

const QStringList PackageEditorState_DrawTextBase::getTextSuggestions()
    const noexcept {
  if (mMode == Mode::TEXT) {
    return {
        "{{NAME}}",  //
        "{{VALUE}}",  //
        "{{BOARD}}",  //
        "{{PROJECT}}",  //
        "{{AUTHOR}}",  //
        "{{VERSION}}",  //
        "{{DATE}}",  //
        "{{TIME}}",  //
    };
  } else {
    return QStringList();
  }
}

PositiveLength PackageEditorState_DrawTextBase::getDefaultHeight(
    const Layer& layer) noexcept {
  if ((layer == Layer::topNames()) || (layer == Layer::botNames()) ||
      (layer == Layer::topValues()) || (layer == Layer::botValues())) {
    return PositiveLength(1000000);  // According library conventions
  } else {
    return PositiveLength(2000000);  // Something reasonable
  }
}

void PackageEditorState_DrawTextBase::setHeight(
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

void PackageEditorState_DrawTextBase::resetHeight() noexcept {
  setHeight(getDefaultHeight(mCurrentProperties.getLayer()));
}

UnsignedLength
    PackageEditorState_DrawTextBase::getDefaultStrokeWidth() noexcept {
  return UnsignedLength(200000);
}

void PackageEditorState_DrawTextBase::setStrokeWidth(
    const UnsignedLength& width) noexcept {
  if (!mCurrentProperties.setStrokeWidth(width)) {
    return;
  }

  mSettings.set(
      "stroke_width/by_layer/" % mCurrentProperties.getLayer().getId(),
      mCurrentProperties.getStrokeWidth());
  emit strokeWidthChanged(mCurrentProperties.getStrokeWidth());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setStrokeWidth(mCurrentProperties.getStrokeWidth(), true);
  }
}

void PackageEditorState_DrawTextBase::resetStrokeWidth() noexcept {
  setStrokeWidth(getDefaultStrokeWidth());
}

void PackageEditorState_DrawTextBase::setAlign(
    const Alignment& align) noexcept {
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

bool PackageEditorState_DrawTextBase::startAddText(const Point& pos) noexcept {
  if (!mContext.currentGraphicsItem) return false;

  try {
    mStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add Footprint Text"));
    mCurrentProperties.setPosition(pos);
    mCurrentText =
        std::make_shared<StrokeText>(Uuid::createRandom(), mCurrentProperties);
    mContext.undoStack.appendToCmdGroup(new CmdStrokeTextInsert(
        mContext.currentFootprint->getStrokeTexts(), mCurrentText));
    mCurrentEditCmd = std::make_unique<CmdStrokeTextEdit>(*mCurrentText);
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentText);
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

bool PackageEditorState_DrawTextBase::finishAddText(const Point& pos) noexcept {
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

bool PackageEditorState_DrawTextBase::abortAddText() noexcept {
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

void PackageEditorState_DrawTextBase::
    loadLayerSpecificClientSettings() noexcept {
  if (mMode == Mode::TEXT) {
    if (auto v = mSettings.get<PositiveLength>(
            "height/by_layer/" % mCurrentProperties.getLayer().getId())) {
      setHeight(*v);
    }
    if (auto v = mSettings.get<UnsignedLength>(
            "stroke_width/by_layer/" % mCurrentProperties.getLayer().getId())) {
      setStrokeWidth(*v);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
