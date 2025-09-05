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
#include "bi_pad.h"

#include "../../../library/cmp/componentsignal.h"
#include "../../../library/dev/device.h"
#include "../../../library/pkg/footprint.h"
#include "../../../library/pkg/package.h"
#include "../../../types/layer.h"
#include "../../../utils/transform.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "../board.h"
#include "../boarddesignrules.h"
#include "bi_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Pad::BI_Pad(BI_NetSegment& netsegment, const BoardPadData& properties)
  : BI_Base(netsegment.getBoard()),
    onEdited(*this),
    mNetSegment(&netsegment),
    mDevice(nullptr),
    mFootprintPad(nullptr),
    mPackagePad(nullptr),
    mComponentSignalInstance(nullptr),
    mProperties(properties),
    mMirrored(false),
    mOnDeviceEditedSlot(*this, &BI_Pad::deviceEdited) {
  updateTransform();
  updateText();
  updateGeometries();

  connect(&mBoard, &Board::designRulesModified, this,
          &BI_Pad::updateGeometries);
  connect(&mBoard, &Board::innerLayerCountChanged, this,
          &BI_Pad::updateGeometries);
}

static BoardPadData convertFootprintPad(const FootprintPad& pad) {
  return BoardPadData(pad.getUuid(), pad.getPosition(), pad.getRotation(),
                      pad.getShape(), pad.getWidth(), pad.getHeight(),
                      pad.getRadius(), pad.getCustomShapeOutline(),
                      pad.getStopMaskConfig(), pad.getSolderPasteConfig(),
                      pad.getCopperClearance(), pad.getComponentSide(),
                      pad.getFunction(), pad.getHoles(), true);
}

BI_Pad::BI_Pad(BI_Device& device, const Uuid& padUuid)
  : BI_Base(device.getBoard()),
    onEdited(*this),
    mNetSegment(nullptr),
    mDevice(&device),
    mFootprintPad(
        mDevice->getLibFootprint().getPads().get(padUuid).get()),  // can throw
    mPackagePad(nullptr),
    mComponentSignalInstance(nullptr),
    mProperties(convertFootprintPad(*mFootprintPad)),
    mMirrored(false),
    mText(),
    mOnDeviceEditedSlot(*this, &BI_Pad::deviceEdited) {
  if (auto pkgPad = mFootprintPad->getPackagePadUuid()) {
    mPackagePad =
        mDevice->getLibPackage().getPads().get(*pkgPad).get();  // can throw

    std::optional<Uuid> cmpSignalUuid = mDevice->getLibDevice()
                                            .getPadSignalMap()
                                            .get(*pkgPad)
                                            ->getSignalUuid();  // can throw
    if (cmpSignalUuid) {
      mComponentSignalInstance =
          mDevice->getComponentInstance().getSignalInstance(*cmpSignalUuid);
      connect(mComponentSignalInstance,
              &ComponentSignalInstance::netSignalChanged, this,
              &BI_Pad::netSignalChanged);
    }
  }

  updateTransform();
  updateText();
  updateGeometries();

  mDevice->onEdited.attach(mOnDeviceEditedSlot);
  connect(&mBoard, &Board::designRulesModified, this,
          &BI_Pad::updateGeometries);
  connect(&mBoard, &Board::innerLayerCountChanged, this,
          &BI_Pad::updateGeometries);
}

BI_Pad::~BI_Pad() noexcept {
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

NetSignal* BI_Pad::getNetSignal() const noexcept {
  if (mNetSegment) {
    return mNetSegment->getNetSignal();
  } else if (mComponentSignalInstance) {
    return mComponentSignalInstance->getNetSignal();
  } else {
    return nullptr;
  }
}

Pad::ComponentSide BI_Pad::getComponentSide() const noexcept {
  if (getMirrored()) {
    return (mProperties.getComponentSide() == Pad::ComponentSide::Top)
        ? Pad::ComponentSide::Bottom
        : Pad::ComponentSide::Top;
  } else {
    return mProperties.getComponentSide();
  }
}

const Layer& BI_Pad::getSolderLayer() const noexcept {
  if (mProperties.isTht()) {
    return (getComponentSide() == Pad::ComponentSide::Bottom)
        ? Layer::topCopper()
        : Layer::botCopper();
  } else {
    return (getComponentSide() == Pad::ComponentSide::Bottom)
        ? Layer::botCopper()
        : Layer::topCopper();
  }
}

bool BI_Pad::isOnLayer(const Layer& layer) const noexcept {
  if (mProperties.isTht()) {
    return layer.isCopper();
  } else {
    return layer == getSolderLayer();
  }
}

TraceAnchor BI_Pad::toTraceAnchor() const noexcept {
  if (mNetSegment) {
    return TraceAnchor::pad(mProperties.getUuid());
  } else {
    return TraceAnchor::footprintPad(mDevice->getComponentInstanceUuid(),
                                     mProperties.getUuid());
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Pad::setPosition(const Point& position) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setPosition(position)) {
    updateTransform();
  }
}

void BI_Pad::setRotation(const Angle& rotation) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setRotation(rotation)) {
    updateTransform();
  }
}

void BI_Pad::setShape(Pad::Shape shape) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setShape(shape)) {
    updateGeometries();
    onEdited.notify(Event::ShapeChanged);
  }
}

void BI_Pad::setWidth(const PositiveLength& width) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setWidth(width)) {
    updateGeometries();
    onEdited.notify(Event::WidthChanged);
  }
}

void BI_Pad::setHeight(const PositiveLength& height) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setHeight(height)) {
    updateGeometries();
    onEdited.notify(Event::HeightChanged);
  }
}

void BI_Pad::setRadius(const UnsignedLimitedRatio& radius) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setRadius(radius)) {
    updateGeometries();
    onEdited.notify(Event::RadiusChanged);
  }
}

void BI_Pad::setCustomShapeOutline(const Path& outline) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setCustomShapeOutline(outline)) {
    updateGeometries();
    onEdited.notify(Event::CustomShapeOutlineChanged);
  }
}

void BI_Pad::setStopMaskConfig(const MaskConfig& config) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setStopMaskConfig(config)) {
    updateGeometries();
    onEdited.notify(Event::StopMaskConfigChanged);
  }
}

void BI_Pad::setSolderPasteConfig(const MaskConfig& config) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setSolderPasteConfig(config)) {
    updateGeometries();
    onEdited.notify(Event::SolderPasteConfigChanged);
  }
}

void BI_Pad::setCopperClearance(const UnsignedLength& clearance) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setCopperClearance(clearance)) {
    invalidatePlanes();
    onEdited.notify(Event::CopperClearanceChanged);
  }
}

void BI_Pad::setComponentSideAndHoles(Pad::ComponentSide side,
                                      const PadHoleList& holes) {
  if (!mNetSegment) return;

  if (holes.isEmpty()) {
    const Layer& smtLayer = (side == Pad::ComponentSide::Bottom)
        ? Layer::botCopper()
        : Layer::topCopper();
    for (const BI_NetLine* nl : mRegisteredNetLines) {
      if (nl->getLayer() != smtLayer) {
        throw LogicError(__FILE__, __LINE__,
                         "Cannot modify pad with traces connected to it.");
      }
    }
  }

  bool modified = false;
  if (mProperties.setComponentSide(side)) {
    onEdited.notify(Event::ComponentSideChanged);
    modified = true;
  }
  if (holes != mProperties.getHoles()) {
    mProperties.getHoles() = holes;
    onEdited.notify(Event::HolesEdited);
    modified = true;
  }
  if (modified) {
    updateGeometries();
  }
}

void BI_Pad::setFunction(Pad::Function function) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setFunction(function)) {
    onEdited.notify(Event::FunctionChanged);
  }
}

void BI_Pad::setLocked(bool locked) noexcept {
  if (!mNetSegment) return;

  if (mProperties.setLocked(locked)) {
    onEdited.notify(Event::LockedChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Pad::addToBoard() {
  if (isAddedToBoard() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->registerFootprintPad(*this);  // can throw
  }
  netSignalChanged(nullptr, getNetSignal());
  BI_Base::addToBoard();
  invalidatePlanes();
}

void BI_Pad::removeFromBoard() {
  if ((!isAddedToBoard()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->unregisterFootprintPad(*this);  // can throw
  }
  netSignalChanged(getNetSignal(), nullptr);
  BI_Base::removeFromBoard();
  invalidatePlanes();
}

void BI_Pad::registerNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (mRegisteredNetLines.contains(&netline)) ||
      (netline.getBoard() != mBoard) ||
      (mNetSegment && (&netline.getNetSegment() != mNetSegment))) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (netline.getNetSegment().getNetSignal() != getNetSignal()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Trace of net \"%1\" is not allowed to be connected to "
                "pad \"%2\" of device \"%3\" (%4) since it is connected to the "
                "net \"%5\".")
            .arg(netline.getNetSegment().getNetNameToDisplay(),
                 getPadNameOrUuid(), getComponentInstanceName(),
                 getLibraryDeviceName(), getNetSignalName()));
  }
  if (!isOnLayer(netline.getLayer())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Trace on layer \"%1\" cannot be connected to the pad \"%2\" "
                "of device \"%3\" (%4) since it is on layer \"%5\".")
            .arg(netline.getLayer().getNameTr(), getPadNameOrUuid(),
                 getComponentInstanceName(), getLibraryDeviceName(),
                 getSolderLayer().getNameTr()));
  }
  // Net segment of board pads is already checked above, but net segment of
  // footprint pads will be checked here.
  if (!mNetSegment) {
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
  }
  mRegisteredNetLines.insert(&netline);
  updateGeometries();
}

void BI_Pad::unregisterNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  updateGeometries();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Pad::deviceEdited(const BI_Device& obj,
                          BI_Device::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Device::Event::BoardLayersChanged:
      break;  // Already handled by a signal-slot connection to the board.
    case BI_Device::Event::PositionChanged:
    case BI_Device::Event::RotationChanged:
    case BI_Device::Event::MirroredChanged:
      updateTransform();
      break;
    case BI_Device::Event::StopMaskOffsetsChanged:
      break;
    default: {
      qWarning() << "Unhandled switch-case in BI_Pad::deviceEdited():"
                 << static_cast<int>(event);
      break;
    }
  }
}

void BI_Pad::netSignalChanged(NetSignal* from, NetSignal* to) {
  Q_ASSERT(!isUsed());  // no netlines must be connected when netsignal changes!
  if (from) {
    disconnect(from, &NetSignal::nameChanged, this, &BI_Pad::updateText);
    mBoard.scheduleAirWiresRebuild(from);
  }
  if (to) {
    connect(to, &NetSignal::nameChanged, this, &BI_Pad::updateText);
    mBoard.scheduleAirWiresRebuild(to);
  }
  invalidatePlanes();
  updateText();
}

void BI_Pad::updateTransform() noexcept {
  Point position;
  Angle rotation;
  bool mirrored;

  if (mDevice) {
    Transform transform(*mDevice);
    position = transform.map(mProperties.getPosition());
    rotation = transform.mapMirrorable(mProperties.getRotation());
    mirrored = mDevice->getMirrored();
  } else {
    position = mProperties.getPosition();
    rotation = mProperties.getRotation();
    mirrored = false;
  }

  if (position != mPosition) {
    mPosition = position;
    mBoard.scheduleAirWiresRebuild(getNetSignal());
    onEdited.notify(Event::PositionChanged);
    foreach (BI_NetLine* netLine, mRegisteredNetLines) {
      netLine->updatePositions();
    }
    invalidatePlanes();
  }
  if (rotation != mRotation) {
    mRotation = rotation;
    onEdited.notify(Event::RotationChanged);
    invalidatePlanes();
  }
  if (mirrored != mMirrored) {
    mMirrored = mirrored;
    onEdited.notify(Event::MirroredChanged);
    updateGeometries();
  }
}

void BI_Pad::updateText() noexcept {
  QString text;
  if (mPackagePad) {
    text += *mPackagePad->getName();
  }
  // Show the component signal name too if it differs from the pad name,
  // because it is much more expressive. To avoid long texts, only display the
  // text up to the first "/" as it is usually unique already for the device.
  if (mComponentSignalInstance) {
    const QString fullName =
        *mComponentSignalInstance->getCompSignal().getName();
    const int sepPos = fullName.indexOf("/", 1);  // Ignore leading slash.
    const QString shortName = (sepPos != -1) ? fullName.left(sepPos) : fullName;
    if ((fullName != text) && (shortName != text)) {
      text += ":" % shortName;
    }
  }
  // To avoid too small text size, truncate text.
  if (text.count() > 8) {
    text = text.left(6) % "…";
  }
  // Show the net name on the next line to avoid too long texts.
  if (NetSignal* signal = getNetSignal()) {
    if (!text.isEmpty()) text += "\n";
    text += *signal->getName();
  }
  if (text != mText) {
    mText = text;
    onEdited.notify(Event::TextChanged);
  }
}

void BI_Pad::updateGeometries() noexcept {
  const QSet<const Layer*> layers = mBoard.getCopperLayers() +
      QSet<const Layer*>{
          &Layer::topStopMask(),
          &Layer::botStopMask(),
          &Layer::topSolderPaste(),
          &Layer::botSolderPaste(),
      };

  QHash<const Layer*, QList<PadGeometry>> geometries;
  foreach (const Layer* layer, layers) {
    geometries.insert(layer, getGeometryOnLayer(*layer));
  }

  if (geometries != mGeometries) {
    mGeometries = geometries;
    onEdited.notify(Event::GeometriesChanged);
    mBoard.invalidatePlanes();
  }
}

void BI_Pad::invalidatePlanes() noexcept {
  if (mProperties.isTht()) {
    mBoard.invalidatePlanes();
  } else {
    mBoard.invalidatePlanes(&getSolderLayer());
  }
}

QString BI_Pad::getLibraryDeviceName() const noexcept {
  return mDevice ? *mDevice->getLibDevice().getNames().getDefaultValue()
                 : QString();
}

QString BI_Pad::getComponentInstanceName() const noexcept {
  return mDevice ? *mDevice->getComponentInstance().getName() : QString();
}

QString BI_Pad::getPadNameOrUuid() const noexcept {
  return mPackagePad ? *mPackagePad->getName() : mProperties.getUuid().toStr();
}

QString BI_Pad::getNetSignalName() const noexcept {
  if (const NetSignal* signal = getNetSignal()) {
    return *signal->getName();
  } else {
    return QString();
  }
}

UnsignedLength BI_Pad::getSizeForMaskOffsetCalculaton() const noexcept {
  if (mProperties.getShape() == Pad::Shape::Custom) {
    // Width/height of the shape are not directly known and difficulat/heavy to
    // determine. So let's consider the pad as small to always get the smallest
    // offset from the design rule. Not perfect, but should be good enough.
    return UnsignedLength(0);
  } else {
    return positiveToUnsigned(
        std::min(mProperties.getWidth(), mProperties.getHeight()));
  }
}

QList<PadGeometry> BI_Pad::getGeometryOnLayer(
    const Layer& layer) const noexcept {
  if (layer.isCopper()) {
    return getGeometryOnCopperLayer(layer);
  }

  QList<PadGeometry> result;
  std::optional<Length> offset;
  if (layer.isStopMask()) {
    const MaskConfig& cfg = mProperties.getStopMaskConfig();
    const bool isThtSolderSide =
        (layer.isTop() == (getComponentSide() == Pad::ComponentSide::Bottom));
    const bool autoAnnularRing =
        mBoard.getDesignRules().getPadCmpSideAutoAnnularRing();
    if (cfg.isEnabled() && cfg.getOffset() &&
        ((!mProperties.isTht()) || isThtSolderSide || (!autoAnnularRing))) {
      // Use offset configured in pad.
      offset = *cfg.getOffset();
    } else if (cfg.isEnabled()) {
      // Use offset from design rules.
      offset = *mBoard.getDesignRules().getStopMaskClearance().calcValue(
          *getSizeForMaskOffsetCalculaton());
    }
  } else if (layer.isSolderPaste()) {
    const MaskConfig& cfg = mProperties.getSolderPasteConfig();
    const bool isThtSolderSide =
        (layer.isTop() == (getComponentSide() == Pad::ComponentSide::Bottom));
    if (cfg.isEnabled() && ((!mProperties.isTht()) || isThtSolderSide)) {
      if (const std::optional<Length>& manualOffset = cfg.getOffset()) {
        // Use offset configured in pad.
        offset = -(*manualOffset);
      } else {
        // Use offset from design rules.
        offset = -mBoard.getDesignRules().getSolderPasteClearance().calcValue(
            *getSizeForMaskOffsetCalculaton());
      }
    }
  }
  if (offset) {
    const Layer& copperLayer =
        layer.isTop() ? Layer::topCopper() : Layer::botCopper();
    foreach (const PadGeometry& pg, getGeometryOnCopperLayer(copperLayer)) {
      result.append(pg.withoutHoles().withOffset(*offset));
    }
  }
  return result;
}

QList<PadGeometry> BI_Pad::getGeometryOnCopperLayer(
    const Layer& layer) const noexcept {
  Q_ASSERT(layer.isCopper());

  // Determine pad shape.
  bool fullShape = false;
  bool autoAnnular = false;
  bool minimalAnnular = false;
  const Layer& componentSideLayer =
      (getComponentSide() == Pad::ComponentSide::Top) ? Layer::topCopper()
                                                      : Layer::botCopper();
  if (mProperties.isTht()) {
    const Layer& solderSideLayer =
        (getComponentSide() == Pad::ComponentSide::Top) ? Layer::botCopper()
                                                        : Layer::topCopper();
    const bool fullComponentSide =
        !mBoard.getDesignRules().getPadCmpSideAutoAnnularRing();
    const bool fullInner =
        !mBoard.getDesignRules().getPadInnerAutoAnnularRing();
    if ((layer == solderSideLayer) ||  // solder side
        (fullComponentSide &&
         (layer == componentSideLayer)) ||  // component side
        (fullInner && layer.isInner())) {  // inner layer
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
    result.append(mProperties.getGeometry());
  } else if (autoAnnular || minimalAnnular) {
    for (const PadHole& hole : mProperties.getHoles()) {
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

bool BI_Pad::isConnectedOnLayer(const Layer& layer) const noexcept {
  foreach (const BI_NetLine* line, mRegisteredNetLines) {
    if (line->getLayer() == layer) {
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
