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
#include "bi_footprint.h"

#include "../../circuit/circuit.h"
#include "../../library/projectlibrary.h"
#include "../../project.h"
#include "../board.h"
#include "../cmd/cmdfootprintstroketextsreset.h"
#include "bi_device.h"
#include "bi_footprintpad.h"

#include <librepcb/common/font/strokefontpool.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/scopeguardlist.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/package.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Footprint::BI_Footprint(BI_Device& device, const BI_Footprint& other)
  : BI_Base(device.getBoard()), mDevice(device) {
  foreach (const BI_StrokeText* text, other.mStrokeTexts) {
    addStrokeText(*new BI_StrokeText(mBoard, *text));
  }
  init();
}

BI_Footprint::BI_Footprint(BI_Device& device, const SExpression& node,
                           const Version& fileFormat)
  : BI_Base(device.getBoard()), mDevice(device) {
  foreach (const SExpression& node, node.getChildren("stroke_text")) {
    addStrokeText(*new BI_StrokeText(mBoard, node, fileFormat));  // can throw
  }
  init();
}

BI_Footprint::BI_Footprint(BI_Device& device)
  : BI_Base(device.getBoard()), mDevice(device) {
  resetStrokeTextsToLibraryFootprint();
  init();
}

void BI_Footprint::init() {
  // create graphics item
  mGraphicsItem.reset(new BGI_Footprint(*this));
  mGraphicsItem->setPos(mDevice.getPosition().toPxQPointF());
  updateGraphicsItemTransform();

  // load pads
  const library::Package& libPkg = mDevice.getLibPackage();
  const library::Device& libDev = mDevice.getLibDevice();
  for (const library::FootprintPad& libPad : getLibFootprint().getPads()) {
    if (mPads.contains(libPad.getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("The footprint pad UUID \"%1\" is defined multiple times.")
              .arg(libPad.getUuid().toStr()));
    }
    if (libPad.getPackagePadUuid() &&
        (!libPkg.getPads().contains(*libPad.getPackagePadUuid()))) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Pad \"%1\" not found in package \"%2\".")
                             .arg(libPad.getPackagePadUuid()->toStr(),
                                  libPkg.getUuid().toStr()));
    }
    if (libPad.getPackagePadUuid() &&
        (!libDev.getPadSignalMap().contains(*libPad.getPackagePadUuid()))) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Package pad \"%1\" not found in "
                                 "pad-signal-map of device \"%2\".")
                             .arg(libPad.getPackagePadUuid()->toStr(),
                                  libDev.getUuid().toStr()));
    }
    BI_FootprintPad* pad = new BI_FootprintPad(*this, libPad.getUuid());
    mPads.insert(libPad.getUuid(), pad);
  }

  // connect to the "attributes changed" signal of device instance
  connect(&mDevice, &BI_Device::attributesChanged, this,
          &BI_Footprint::deviceInstanceAttributesChanged);
  connect(&mDevice, &BI_Device::moved, this,
          &BI_Footprint::deviceInstanceMoved);
  connect(&mDevice, &BI_Device::rotated, this,
          &BI_Footprint::deviceInstanceRotated);
  connect(&mDevice, &BI_Device::mirrored, this,
          &BI_Footprint::deviceInstanceMirrored);
}

BI_Footprint::~BI_Footprint() noexcept {
  qDeleteAll(mPads);
  mPads.clear();
  qDeleteAll(mStrokeTexts);
  mStrokeTexts.clear();
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& BI_Footprint::getComponentInstanceUuid() const noexcept {
  return mDevice.getComponentInstanceUuid();
}

const library::Footprint& BI_Footprint::getLibFootprint() const noexcept {
  return mDevice.getLibFootprint();
}

const Angle& BI_Footprint::getRotation() const noexcept {
  return mDevice.getRotation();
}

bool BI_Footprint::isUsed() const noexcept {
  foreach (const BI_FootprintPad* pad, mPads) {
    if (pad->isUsed()) return true;
  }
  return false;
}

QRectF BI_Footprint::getBoundingRect() const noexcept {
  return mGraphicsItem->sceneTransform().mapRect(mGraphicsItem->boundingRect());
}

/*******************************************************************************
 *  StrokeText Methods
 ******************************************************************************/

void BI_Footprint::addStrokeText(BI_StrokeText& text) {
  if ((mStrokeTexts.contains(&text)) || (&text.getBoard() != &mBoard)) {
    throw LogicError(__FILE__, __LINE__);
  }

  // set font and attribute substitutor for all text items
  // TODO: move this to BI_StrokeText?!
  text.setFootprint(this);
  text.getText().setAttributeProvider(this);
  text.getText().setFont(&getProject().getStrokeFonts().getFont(
      mBoard.getDefaultFontName()));  // can throw

  if (isAddedToBoard()) {
    text.addToBoard();  // can throw
  }
  mStrokeTexts.append(&text);
}

void BI_Footprint::removeStrokeText(BI_StrokeText& text) {
  if (!mStrokeTexts.contains(&text)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isAddedToBoard()) {
    text.removeFromBoard();  // can throw
  }
  mStrokeTexts.removeOne(&text);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Footprint::resetStrokeTextsToLibraryFootprint() {
  CmdFootprintStrokeTextsReset cmd(*this);
  cmd.execute();  // can throw
}

void BI_Footprint::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl(mPads.count());
  foreach (BI_FootprintPad* pad, mPads) {
    pad->addToBoard();  // can throw
    sgl.add([pad]() { pad->removeFromBoard(); });
  }
  foreach (BI_StrokeText* text, mStrokeTexts) {
    text->addToBoard();  // can throw
    sgl.add([text]() { text->removeFromBoard(); });
  }
  BI_Base::addToBoard(mGraphicsItem.data());
  sgl.dismiss();
}

void BI_Footprint::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl(mPads.count());
  foreach (BI_FootprintPad* pad, mPads) {
    pad->removeFromBoard();  // can throw
    sgl.add([pad]() { pad->addToBoard(); });
  }
  foreach (BI_StrokeText* text, mStrokeTexts) {
    text->removeFromBoard();  // can throw
    sgl.add([text]() { text->addToBoard(); });
  }
  BI_Base::removeFromBoard(mGraphicsItem.data());
  sgl.dismiss();
}

void BI_Footprint::serialize(SExpression& root) const {
  serializePointerContainerUuidSorted(root, mStrokeTexts, "stroke_text");
}

/*******************************************************************************
 *  Helper Methods
 ******************************************************************************/

Point BI_Footprint::mapToScene(const Point& relativePos) const noexcept {
  if (mDevice.getIsMirrored()) {
    return (mDevice.getPosition() + relativePos)
        .rotated(mDevice.getRotation(), mDevice.getPosition())
        .mirrored(Qt::Horizontal, mDevice.getPosition());
  } else {
    return (mDevice.getPosition() + relativePos)
        .rotated(mDevice.getRotation(), mDevice.getPosition());
  }
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QVector<const AttributeProvider*> BI_Footprint::getAttributeProviderParents()
    const noexcept {
  return QVector<const AttributeProvider*>{&mDevice};
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

const Point& BI_Footprint::getPosition() const noexcept {
  return mDevice.getPosition();
}

bool BI_Footprint::getIsMirrored() const noexcept {
  return mDevice.getIsMirrored();
}

QPainterPath BI_Footprint::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

bool BI_Footprint::isSelectable() const noexcept {
  return mGraphicsItem->isSelectable();
}

void BI_Footprint::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->update();
  foreach (BI_FootprintPad* pad, mPads)
    pad->setSelected(selected);
  foreach (BI_StrokeText* text, mStrokeTexts)
    text->setSelected(selected);
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BI_Footprint::deviceInstanceAttributesChanged() {
  mGraphicsItem->updateCacheAndRepaint();
  emit attributesChanged();
}

void BI_Footprint::deviceInstanceMoved(const Point& pos) {
  mGraphicsItem->setPos(pos.toPxQPointF());
  mGraphicsItem->updateCacheAndRepaint();
  foreach (BI_FootprintPad* pad, mPads) {
    pad->updatePosition();
    mBoard.scheduleAirWiresRebuild(pad->getCompSigInstNetSignal());
  }
  foreach (BI_StrokeText* text, mStrokeTexts) { text->updateGraphicsItems(); }
}

void BI_Footprint::deviceInstanceRotated(const Angle& rot) {
  Q_UNUSED(rot);
  updateGraphicsItemTransform();
  mGraphicsItem->updateCacheAndRepaint();
  foreach (BI_FootprintPad* pad, mPads) {
    pad->updatePosition();
    mBoard.scheduleAirWiresRebuild(pad->getCompSigInstNetSignal());
  }
}

void BI_Footprint::deviceInstanceMirrored(bool mirrored) {
  Q_UNUSED(mirrored);
  updateGraphicsItemTransform();
  mGraphicsItem->updateCacheAndRepaint();
  foreach (BI_FootprintPad* pad, mPads) {
    pad->updatePosition();
    mBoard.scheduleAirWiresRebuild(pad->getCompSigInstNetSignal());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Footprint::updateGraphicsItemTransform() noexcept {
  QTransform t;
  if (mDevice.getIsMirrored()) t.scale(qreal(-1), qreal(1));
  t.rotate(-mDevice.getRotation().toDeg());
  mGraphicsItem->setTransform(t);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
