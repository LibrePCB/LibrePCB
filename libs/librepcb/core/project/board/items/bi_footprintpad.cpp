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
#include "bi_footprintpad.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../library/dev/device.h"
#include "../../../library/pkg/footprint.h"
#include "../../../library/pkg/package.h"
#include "../../../utils/transform.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "../board.h"
#include "../boarddesignrules.h"
#include "../boardlayerstack.h"
#include "bi_device.h"
#include "bi_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_FootprintPad::BI_FootprintPad(BI_Device& device, const Uuid& padUuid)
  : BI_Base(device.getBoard()),
    onEdited(*this),
    mDevice(device),
    mFootprintPad(nullptr),
    mPackagePad(nullptr),
    mComponentSignalInstance(nullptr),
    mOnDeviceEditedSlot(*this, &BI_FootprintPad::deviceEdited) {
  mFootprintPad =
      mDevice.getLibFootprint().getPads().get(padUuid).get();  // can throw
  if (mFootprintPad->getPackagePadUuid()) {
    mPackagePad = mDevice.getLibPackage()
                      .getPads()
                      .get(*mFootprintPad->getPackagePadUuid())
                      .get();  // can throw

    tl::optional<Uuid> cmpSignalUuid =
        mDevice.getLibDevice()
            .getPadSignalMap()
            .get(*mFootprintPad->getPackagePadUuid())
            ->getSignalUuid();  // can throw
    if (cmpSignalUuid) {
      mComponentSignalInstance =
          mDevice.getComponentInstance().getSignalInstance(*cmpSignalUuid);
      connect(mComponentSignalInstance,
              &ComponentSignalInstance::netSignalChanged, this,
              &BI_FootprintPad::netSignalChanged);
    }
  }

  if (NetSignal* netsignal = getCompSigInstNetSignal()) {
    connect(netsignal, &NetSignal::nameChanged, this,
            &BI_FootprintPad::updateText);
  }

  updateTransform();
  updateText();
  updateGeometries();

  mDevice.onEdited.attach(mOnDeviceEditedSlot);
  connect(&mBoard, &Board::designRulesModified, this,
          &BI_FootprintPad::updateGeometries);
}

BI_FootprintPad::~BI_FootprintPad() {
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& BI_FootprintPad::getLibPadUuid() const noexcept {
  return mFootprintPad->getUuid();
}

FootprintPad::ComponentSide BI_FootprintPad::getComponentSide() const noexcept {
  if (getMirrored()) {
    return (mFootprintPad->getComponentSide() ==
            FootprintPad::ComponentSide::Top)
        ? FootprintPad::ComponentSide::Bottom
        : FootprintPad::ComponentSide::Top;
  } else {
    return mFootprintPad->getComponentSide();
  }
}

QString BI_FootprintPad::getLayerName() const noexcept {
  if (getMirrored())
    return GraphicsLayer::getMirroredLayerName(mFootprintPad->getLayerName());
  else
    return mFootprintPad->getLayerName();
}

bool BI_FootprintPad::isOnLayer(const QString& layerName) const noexcept {
  if (getMirrored()) {
    return mFootprintPad->isOnLayer(
        GraphicsLayer::getMirroredLayerName(layerName));
  } else {
    return mFootprintPad->isOnLayer(layerName);
  }
}

NetSignal* BI_FootprintPad::getCompSigInstNetSignal() const noexcept {
  if (mComponentSignalInstance) {
    return mComponentSignalInstance->getNetSignal();
  } else {
    return nullptr;
  }
}

TraceAnchor BI_FootprintPad::toTraceAnchor() const noexcept {
  return TraceAnchor::pad(mDevice.getComponentInstanceUuid(),
                          mFootprintPad->getUuid());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_FootprintPad::addToBoard() {
  if (isAddedToBoard() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->registerFootprintPad(*this);  // can throw
  }
  netSignalChanged(nullptr, getCompSigInstNetSignal());
  BI_Base::addToBoard();
}

void BI_FootprintPad::removeFromBoard() {
  if ((!isAddedToBoard()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->unregisterFootprintPad(*this);  // can throw
  }
  netSignalChanged(getCompSigInstNetSignal(), nullptr);
  BI_Base::removeFromBoard();
}

void BI_FootprintPad::registerNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (mRegisteredNetLines.contains(&netline)) ||
      (netline.getBoard() != mBoard)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (netline.getNetSegment().getNetSignal() != getCompSigInstNetSignal()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Trace of net \"%1\" is not allowed to be connected to "
                "pad \"%2\" of device \"%3\" (%4) since it is connected to the "
                "net \"%5\".")
            .arg(netline.getNetSegment().getNetNameToDisplay(),
                 getPadNameOrUuid(), getComponentInstanceName(),
                 getLibraryDeviceName(), getNetSignalName()));
  }
  if (!isOnLayer(netline.getLayer().getName())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Trace on layer \"%1\" cannot be connected to the pad \"%2\" "
                "of device \"%3\" (%4) since it is on layer \"%5\".")
            .arg(netline.getLayer().getName(), getPadNameOrUuid(),
                 getComponentInstanceName(), getLibraryDeviceName(),
                 getLayerName()));
  }
  foreach (const BI_NetLine* l, mRegisteredNetLines) {
    if (&l->getNetSegment() != &netline.getNetSegment()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There are traces from multiple net segments connected to "
                  "the pad \"%1\" of device \"%2\" (%3).")
              .arg(getPadNameOrUuid(), getComponentInstanceName(),
                   getLibraryDeviceName()));
    }
  }
  mRegisteredNetLines.insert(&netline);
  updateGeometries();
}

void BI_FootprintPad::unregisterNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  updateGeometries();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BI_FootprintPad::deviceEdited(const BI_Device& obj,
                                   BI_Device::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Device::Event::PositionChanged:
    case BI_Device::Event::RotationChanged:
    case BI_Device::Event::MirroredChanged:
      updateTransform();
      break;
    case BI_Device::Event::StopMaskOffsetsChanged:
      break;
    default: {
      qWarning() << "Unhandled switch-case in BI_FootprintPad::deviceEdited():"
                 << static_cast<int>(event);
      break;
    }
  }
}

void BI_FootprintPad::netSignalChanged(NetSignal* from, NetSignal* to) {
  Q_ASSERT(!isUsed());  // no netlines must be connected when netsignal changes!
  if (from) {
    disconnect(from, &NetSignal::nameChanged, this,
               &BI_FootprintPad::updateText);
    mBoard.scheduleAirWiresRebuild(from);
  }
  if (to) {
    connect(to, &NetSignal::nameChanged, this, &BI_FootprintPad::updateText);
    mBoard.scheduleAirWiresRebuild(to);
  }
  updateText();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_FootprintPad::updateTransform() noexcept {
  Transform transform(mDevice);
  const Point position = transform.map(mFootprintPad->getPosition());
  const Angle rotation = transform.map(mFootprintPad->getRotation());
  const bool mirrored = mDevice.getMirrored();
  if (position != mPosition) {
    mPosition = position;
    mBoard.scheduleAirWiresRebuild(getCompSigInstNetSignal());
    onEdited.notify(Event::PositionChanged);
    foreach (BI_NetLine* netLine, mRegisteredNetLines) {
      netLine->updatePositions();
    }
  }
  if (rotation != mRotation) {
    mRotation = rotation;
    onEdited.notify(Event::RotationChanged);
  }
  if (mirrored != mMirrored) {
    mMirrored = mirrored;
    onEdited.notify(Event::MirroredChanged);
    updateGeometries();
  }
}

void BI_FootprintPad::updateText() noexcept {
  QString text;
  if (mPackagePad) {
    text += *mPackagePad->getName();
  }
  if (NetSignal* signal = getCompSigInstNetSignal()) {
    text += ": " % *signal->getName();
  }
  if (text != mText) {
    mText = text;
    onEdited.notify(Event::TextChanged);
  }
}

void BI_FootprintPad::updateGeometries() noexcept {
  QSet<QString> layers = {
      GraphicsLayer::sTopStopMask,    GraphicsLayer::sBotStopMask,
      GraphicsLayer::sTopSolderPaste, GraphicsLayer::sBotSolderPaste,
      GraphicsLayer::sTopCopper,      GraphicsLayer::sBotCopper,
  };
  for (int i = 1; i <= mBoard.getLayerStack().getInnerLayerCount(); ++i) {
    layers.insert(GraphicsLayer::getInnerLayerName(i));
  }

  QMap<QString, QList<PadGeometry>> geometries;
  foreach (const QString& layer, layers) {
    geometries.insert(layer, getGeometryOnLayer(layer));
  }

  if (geometries != mGeometries) {
    mGeometries = geometries;
    onEdited.notify(Event::GeometriesChanged);
  }
}

QString BI_FootprintPad::getLibraryDeviceName() const noexcept {
  return *mDevice.getLibDevice().getNames().getDefaultValue();
}

QString BI_FootprintPad::getComponentInstanceName() const noexcept {
  return *mDevice.getComponentInstance().getName();
}

QString BI_FootprintPad::getPadNameOrUuid() const noexcept {
  return mPackagePad ? *mPackagePad->getName()
                     : mFootprintPad->getUuid().toStr();
}

QString BI_FootprintPad::getNetSignalName() const noexcept {
  if (const NetSignal* signal = getCompSigInstNetSignal()) {
    return *signal->getName();
  } else {
    return QString();
  }
}

UnsignedLength BI_FootprintPad::getSizeForMaskOffsetCalculaton() const
    noexcept {
  if (mFootprintPad->getShape() == FootprintPad::Shape::Custom) {
    // Width/height of the shape are not directly known and difficulat/heavy to
    // determine. So let's consider the pad as small to always get the smallest
    // offset from the design rule. Not perfect, but should be good enough.
    return UnsignedLength(0);
  } else {
    return positiveToUnsigned(
        std::min(mFootprintPad->getWidth(), mFootprintPad->getHeight()));
  }
}

QList<PadGeometry> BI_FootprintPad::getGeometryOnLayer(
    const QString& layer) const noexcept {
  if (GraphicsLayer::isCopperLayer(layer)) {
    return getGeometryOnCopperLayer(layer);
  }

  QList<PadGeometry> result;
  tl::optional<Length> offset;
  if ((layer == GraphicsLayer::sTopStopMask) ||
      (layer == GraphicsLayer::sBotStopMask)) {
    offset = *mBoard.getDesignRules().getStopMaskClearance().calcValue(
        *getSizeForMaskOffsetCalculaton());
  } else if ((!mFootprintPad->isTht()) &&
             ((layer == GraphicsLayer::sTopSolderPaste) ||
              (layer == GraphicsLayer::sBotSolderPaste))) {
    offset = -mBoard.getDesignRules().getSolderPasteClearance().calcValue(
        *getSizeForMaskOffsetCalculaton());
  }
  if (offset) {
    const QString copperLayer = GraphicsLayer::isTopLayer(layer)
        ? GraphicsLayer::sTopCopper
        : GraphicsLayer::sBotCopper;
    foreach (const PadGeometry& pg, getGeometryOnCopperLayer(copperLayer)) {
      result.append(pg.withoutHoles().withOffset(*offset));
    }
  }
  return result;
}

QList<PadGeometry> BI_FootprintPad::getGeometryOnCopperLayer(
    const QString& layer) const noexcept {
  Q_ASSERT(GraphicsLayer::isCopperLayer(layer));

  // Determine pad shape.
  bool fullShape = false;
  bool autoAnnular = false;
  bool minimalAnnular = false;
  const QString componentSideLayer =
      (getComponentSide() == FootprintPad::ComponentSide::Top)
      ? GraphicsLayer::sTopCopper
      : GraphicsLayer::sBotCopper;
  if (mFootprintPad->isTht()) {
    const QString solderSideLayer =
        (getComponentSide() == FootprintPad::ComponentSide::Top)
        ? GraphicsLayer::sBotCopper
        : GraphicsLayer::sTopCopper;
    const bool fullComponentSide =
        !mBoard.getDesignRules().getPadCmpSideAutoAnnularRing();
    const bool fullInner =
        !mBoard.getDesignRules().getPadInnerAutoAnnularRing();
    if ((layer == solderSideLayer) ||  // solder side
        (fullComponentSide &&
         (layer == componentSideLayer)) ||  // component side
        (fullInner && GraphicsLayer::isInnerLayer(layer))) {  // inner layer
      fullShape = true;
    } else if (isConnectedOnLayer(layer)) {
      autoAnnular = true;
    } else {
      minimalAnnular = true;
    }
  } else if (layer == componentSideLayer) {
    fullShape = true;
  }

  // Build geometry.
  QList<PadGeometry> result;
  if (fullShape) {
    result.append(mFootprintPad->getGeometry());
  } else if (autoAnnular || minimalAnnular) {
    for (const PadHole& hole : mFootprintPad->getHoles()) {
      const UnsignedLength annularWidth = autoAnnular
          ? mBoard.getDesignRules().getPadAnnularRing().calcValue(
                *hole.getDiameter())
          : mBoard.getDesignRules()
                .getPadAnnularRing()
                .getMinValue();  // Min. Annular
      result.append(PadGeometry::stroke(
          hole.getDiameter() + annularWidth + annularWidth, hole.getPath(),
          PadHoleList{std::make_shared<PadHole>(hole)}));
    }
  }
  return result;
}

bool BI_FootprintPad::isConnectedOnLayer(const QString& layer) const noexcept {
  foreach (const BI_NetLine* line, mRegisteredNetLines) {
    if (line->getLayer().getName() == layer) {
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
