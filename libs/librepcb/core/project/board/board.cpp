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
#include "board.h"

#include "../../3d/scenedata3d.h"
#include "../../application.h"
#include "../../exceptions.h"
#include "../../geometry/polygon.h"
#include "../../library/cmp/component.h"
#include "../../library/dev/device.h"
#include "../../library/pkg/footprint.h"
#include "../../library/pkg/package.h"
#include "../../library/pkg/packagemodel.h"
#include "../../serialization/sexpression.h"
#include "../../types/lengthunit.h"
#include "../../types/pcbcolor.h"
#include "../../utils/scopeguardlist.h"
#include "../../utils/toolbox.h"
#include "../circuit/circuit.h"
#include "../circuit/componentinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "boardairwiresbuilder.h"
#include "boarddesignrules.h"
#include "boardfabricationoutputsettings.h"
#include "drc/boarddesignrulechecksettings.h"
#include "items/bi_airwire.h"
#include "items/bi_device.h"
#include "items/bi_footprintpad.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_via.h"
#include "items/bi_zone.h"

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Board::Board(Project& project,
             std::unique_ptr<TransactionalDirectory> directory,
             const QString& directoryName, const Uuid& uuid,
             const ElementName& name)
  : QObject(&project),
    mProject(project),
    mDirectoryName(directoryName),
    mDirectory(std::move(directory)),
    mIsAddedToProject(false),
    mDesignRules(new BoardDesignRules()),
    mDrcSettings(new BoardDesignRuleCheckSettings()),
    mFabricationOutputSettings(new BoardFabricationOutputSettings()),
    mUuid(uuid),
    mName(name),
    mDefaultFontFileName(Application::getDefaultStrokeFontName()),
    mGridInterval(635000),
    mGridUnit(LengthUnit::millimeters()),
    mInnerLayerCount(-1),  // Force update of setter.
    mCopperLayers(),
    mPcbThickness(1600000),  // 1.6mm
    mSolderResist(&PcbColor::green()),
    mSilkscreenColor(&PcbColor::white()),
    mSilkscreenLayersTop({&Layer::topLegend(), &Layer::topNames()}),
    mSilkscreenLayersBot({&Layer::botLegend(), &Layer::botNames()}),
    mDrcMessageApprovalsVersion(Application::getFileFormatVersion()),
    mDrcMessageApprovals(),
    mSupportedDrcMessageApprovals() {
  if (mDirectoryName.isEmpty()) {
    throw LogicError(__FILE__, __LINE__);
  }

  setInnerLayerCount(0);

  // Emit the "attributesChanged" signal when the project has emitted it.
  connect(&mProject, &Project::attributesChanged, this,
          &Board::attributesChanged);
}

Board::~Board() noexcept {
  Q_ASSERT(!mIsAddedToProject);

  // delete all items
  qDeleteAll(mAirWires);
  mAirWires.clear();
  qDeleteAll(mHoles);
  mHoles.clear();
  qDeleteAll(mStrokeTexts);
  mStrokeTexts.clear();
  qDeleteAll(mPolygons);
  mPolygons.clear();
  qDeleteAll(mZones);
  mZones.clear();
  qDeleteAll(mPlanes);
  mPlanes.clear();
  qDeleteAll(mNetSegments);
  mNetSegments.clear();
  qDeleteAll(mDeviceInstances);
  mDeviceInstances.clear();

  mFabricationOutputSettings.reset();
  mDrcSettings.reset();
  mDesignRules.reset();
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

bool Board::isEmpty() const noexcept {
  return (mDeviceInstances.isEmpty() && mNetSegments.isEmpty() &&
          mPlanes.isEmpty() && mZones.isEmpty() && mPolygons.isEmpty() &&
          mStrokeTexts.isEmpty() && mHoles.isEmpty());
}

QList<BI_Base*> Board::getAllItems() const noexcept {
  QList<BI_Base*> items;
  foreach (BI_Device* device, mDeviceInstances) items.append(device);
  foreach (BI_NetSegment* netsegment, mNetSegments) items.append(netsegment);
  foreach (BI_Plane* plane, mPlanes) items.append(plane);
  foreach (BI_Zone* zone, mZones) items.append(zone);
  foreach (BI_Polygon* polygon, mPolygons) items.append(polygon);
  foreach (BI_StrokeText* text, mStrokeTexts) items.append(text);
  foreach (BI_Hole* hole, mHoles) items.append(hole);
  foreach (BI_AirWire* airWire, mAirWires) items.append(airWire);
  return items;
}

std::shared_ptr<SceneData3D> Board::buildScene3D(
    const std::optional<Uuid>& assemblyVariant) const noexcept {
  auto data = std::make_shared<SceneData3D>(
      std::make_shared<TransactionalDirectory>(mProject.getDirectory()), false);
  data->setProjectName(*mProject.getName());
  data->setThickness(mPcbThickness);
  data->setSolderResist(mSolderResist);
  data->setSilkscreen(mSilkscreenColor);
  data->setSilkscreenLayersTop(Toolbox::toSet(mSilkscreenLayersTop));
  data->setSilkscreenLayersBot(Toolbox::toSet(mSilkscreenLayersBot));
  foreach (const BI_Device* obj, mDeviceInstances) {
    const Transform transform(*obj);
    if (auto model = obj->getLibModel()) {
      if (assemblyVariant && obj->isInAssemblyVariant(*assemblyVariant)) {
        const QString stepFile = obj->getLibPackage().getDirectory().getPath() %
            "/" % model->getFileName();
        data->addDevice(obj->getComponentInstanceUuid(), transform, stepFile,
                        obj->getLibFootprint().getModelPosition(),
                        obj->getLibFootprint().getModelRotation(),
                        *obj->getComponentInstance().getName());
      }
    }
    foreach (const BI_FootprintPad* pad, obj->getPads()) {
      const Transform padTransform(*pad);
      auto geometries = pad->getGeometries();
      for (auto it = geometries.begin(); it != geometries.end(); it++) {
        foreach (const PadGeometry& geometry, it.value()) {
          foreach (const Path& outline, geometry.toOutlines()) {
            data->addArea(*it.key(), outline, padTransform);
          }
          for (const PadHole& hole : geometry.getHoles()) {
            data->addHole(hole.getPath(), hole.getDiameter(), true, false,
                          padTransform);
          }
        }
      }
    }
    foreach (const BI_StrokeText* obj, obj->getStrokeTexts()) {
      data->addStroke(obj->getData().getLayer(), obj->getPaths(),
                      *obj->getData().getStrokeWidth(),
                      Transform(obj->getData()));
    }
    for (const Polygon& polygon : obj->getLibFootprint().getPolygons()) {
      data->addPolygon(polygon, transform);
    }
    for (const Circle& circle : obj->getLibFootprint().getCircles()) {
      data->addCircle(circle, transform);
    }
    for (const Hole& hole : obj->getLibFootprint().getHoles()) {
      data->addHole(hole.getPath(), hole.getDiameter(), false, false,
                    transform);
      if (const std::optional<Length>& offset =
              obj->getHoleStopMasks().value(hole.getUuid())) {
        for (const Layer* layer :
             {&Layer::topStopMask(), &Layer::botStopMask()}) {
          data->addStroke(*layer, {*hole.getPath()},
                          (*hole.getDiameter()) + (*offset) + (*offset),
                          Transform(*obj));
        }
      }
    }
  }
  foreach (const BI_Plane* obj, mPlanes) {
    foreach (const Path& fragment, obj->getFragments()) {
      data->addArea(obj->getLayer(), fragment, Transform());
    }
  }
  foreach (const BI_Polygon* obj, mPolygons) {
    data->addPolygon(
        Polygon(obj->getData().getUuid(), obj->getData().getLayer(),
                obj->getData().getLineWidth(), obj->getData().isFilled(),
                obj->getData().isGrabArea(), obj->getData().getPath()),
        Transform());
  }
  foreach (const BI_StrokeText* obj, mStrokeTexts) {
    data->addStroke(obj->getData().getLayer(), obj->getPaths(),
                    *obj->getData().getStrokeWidth(),
                    Transform(obj->getData()));
  }
  foreach (const BI_Hole* obj, mHoles) {
    data->addHole(obj->getData().getPath(), obj->getData().getDiameter(), false,
                  false, Transform());
    if (const std::optional<Length>& offset = obj->getStopMaskOffset()) {
      for (const Layer* layer :
           {&Layer::topStopMask(), &Layer::botStopMask()}) {
        data->addStroke(*layer, {*obj->getData().getPath()},
                        (*obj->getData().getDiameter()) + (*offset) + (*offset),
                        Transform());
      }
    }
  }
  foreach (const BI_NetSegment* netSegment, mNetSegments) {
    foreach (const BI_Via* via, netSegment->getVias()) {
      data->addVia(via->getPosition(), via->getSize(), via->getDrillDiameter(),
                   via->getVia().getStartLayer(), via->getVia().getEndLayer(),
                   via->getStopMaskDiameterTop(),
                   via->getStopMaskDiameterBottom());
    }
    foreach (const BI_NetLine* netLine, netSegment->getNetLines()) {
      data->addStroke(netLine->getLayer(),
                      {Path({Vertex(netLine->getP1().getPosition()),
                             Vertex(netLine->getP2().getPosition())})},
                      *netLine->getWidth(), Transform());
    }
  }
  return data;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Board::setName(const ElementName& name) noexcept {
  if (name != mName) {
    mName = name;
    emit nameChanged(mName);
    emit attributesChanged();
  }
}

void Board::setInnerLayerCount(int count) noexcept {
  if (count != mInnerLayerCount) {
    mInnerLayerCount = count;
    mCopperLayers.clear();
    mCopperLayers.insert(&Layer::topCopper());
    mCopperLayers.insert(&Layer::botCopper());
    for (int i = 1; i <= mInnerLayerCount; ++i) {
      if (const Layer* layer = Layer::innerCopper(i)) {
        mCopperLayers.insert(layer);
        invalidatePlanes(layer);
      }
    }
    emit innerLayerCountChanged();
  }
}

void Board::setDesignRules(const BoardDesignRules& rules) noexcept {
  if (rules != *mDesignRules) {
    *mDesignRules = rules;
    invalidatePlanes();
    emit designRulesModified();
    emit attributesChanged();
  }
}

void Board::setDrcSettings(
    const BoardDesignRuleCheckSettings& settings) noexcept {
  *mDrcSettings = settings;
}

/*******************************************************************************
 *  DRC Message Approval Methods
 ******************************************************************************/

void Board::loadDrcMessageApprovals(
    const Version& version, const QSet<SExpression>& approvals) noexcept {
  mDrcMessageApprovalsVersion = version;
  mDrcMessageApprovals = approvals;
}

bool Board::updateDrcMessageApprovals(QSet<SExpression> approvals,
                                      bool partialRun) noexcept {
  mSupportedDrcMessageApprovals |= approvals;

  // Don't remove obsolete approvals after a partial DRC run because we would
  // loose all approvals which don't occur during the partial run!
  if (partialRun) {
    return false;
  }

  // When running the DRC the first time after a file format upgrade, remove
  // all approvals not occurring anymore to clean up obsolete approvals from
  // the board file.
  if (mDrcMessageApprovalsVersion < Application::getFileFormatVersion()) {
    mDrcMessageApprovalsVersion = Application::getFileFormatVersion();
    mDrcMessageApprovals &= approvals;
    return true;
  }

  // Remove only approvals which disappeared during this session to avoid
  // removing approvals added by newer minor application versions.
  approvals =
      mDrcMessageApprovals - (mSupportedDrcMessageApprovals - approvals);
  if (approvals != mDrcMessageApprovals) {
    mDrcMessageApprovals = approvals;
    return true;
  }

  return false;
}

void Board::setDrcMessageApproved(const SExpression& approval,
                                  bool approved) noexcept {
  if (approved) {
    mDrcMessageApprovals.insert(approval);
  } else {
    mDrcMessageApprovals.remove(approval);
  }
}

/*******************************************************************************
 *  DeviceInstance Methods
 ******************************************************************************/

BI_Device* Board::getDeviceInstanceByComponentUuid(
    const Uuid& uuid) const noexcept {
  return mDeviceInstances.value(uuid, nullptr);
}

void Board::addDeviceInstance(BI_Device& instance) {
  if ((mDeviceInstances.values().contains(&instance)) ||
      (&instance.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mDeviceInstances.value(instance.getComponentInstanceUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a device with the component instance \"%1\"!")
            .arg(instance.getComponentInstanceUuid().toStr()));
  }
  if (mIsAddedToProject) {
    instance.addToBoard();  // can throw
  }
  mDeviceInstances.insert(instance.getComponentInstanceUuid(), &instance);
  emit deviceAdded(instance);
}

void Board::removeDeviceInstance(BI_Device& instance) {
  if (mDeviceInstances.value(instance.getComponentInstanceUuid()) !=
      &instance) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mIsAddedToProject) {
    instance.removeFromBoard();  // can throw
  }
  mDeviceInstances.remove(instance.getComponentInstanceUuid());
  emit deviceRemoved(instance);
}

/*******************************************************************************
 *  NetSegment Methods
 ******************************************************************************/

void Board::addNetSegment(BI_NetSegment& netsegment) {
  if ((mNetSegments.values().contains(&netsegment)) ||
      (&netsegment.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mNetSegments.contains(netsegment.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a netsegment with the UUID \"%1\"!")
            .arg(netsegment.getUuid().toStr()));
  }
  if (mIsAddedToProject) {
    netsegment.addToBoard();  // can throw
  }
  mNetSegments.insert(netsegment.getUuid(), &netsegment);
  emit netSegmentAdded(netsegment);
}

void Board::removeNetSegment(BI_NetSegment& netsegment) {
  if (mNetSegments.value(netsegment.getUuid()) != &netsegment) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mIsAddedToProject) {
    netsegment.removeFromBoard();  // can throw
  }
  mNetSegments.remove(netsegment.getUuid());
  emit netSegmentRemoved(netsegment);
}

/*******************************************************************************
 *  Plane Methods
 ******************************************************************************/

void Board::addPlane(BI_Plane& plane) {
  if ((mPlanes.values().contains(&plane)) || (&plane.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mPlanes.contains(plane.getUuid())) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("There is already a plane with the UUID \"%1\"!")
                           .arg(plane.getUuid().toStr()));
  }
  if (mIsAddedToProject) {
    plane.addToBoard();  // can throw
  }
  mPlanes.insert(plane.getUuid(), &plane);
  emit planeAdded(plane);
}

void Board::removePlane(BI_Plane& plane) {
  if (mPlanes.value(plane.getUuid()) != &plane) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mIsAddedToProject) {
    plane.removeFromBoard();  // can throw
  }
  mPlanes.remove(plane.getUuid());
  emit planeRemoved(plane);
}

void Board::invalidatePlanes(const Layer* layer) noexcept {
  Q_ASSERT((!layer) || (layer->isCopper()));
  if (layer) {
    mScheduledLayersForPlanesRebuild.insert(layer);
  } else {
    mScheduledLayersForPlanesRebuild |= mCopperLayers;
  }
}

void Board::invalidatePlanes(const QSet<const Layer*>& layers) noexcept {
#if defined(QT_NO_DEBUG)
  foreach (const Layer* layer, layers) {
    Q_ASSERT(layer && layer->isCopper());
  }
#endif
  mScheduledLayersForPlanesRebuild |= layers;
}

QSet<const Layer*> Board::takeScheduledLayersForPlanesRebuild(
    const QSet<const Layer*>& layers) noexcept {
  auto result = mScheduledLayersForPlanesRebuild & layers;
  mScheduledLayersForPlanesRebuild -= layers;
  return result;
}

/*******************************************************************************
 *  Zone Methods
 ******************************************************************************/

void Board::addZone(BI_Zone& zone) {
  if ((mZones.values().contains(&zone)) || (&zone.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mZones.contains(zone.getData().getUuid())) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("There is already a zone with the UUID \"%1\"!")
                           .arg(zone.getData().getUuid().toStr()));
  }
  if (mIsAddedToProject) {
    zone.addToBoard();  // can throw
  }
  mZones.insert(zone.getData().getUuid(), &zone);
  emit zoneAdded(zone);
}

void Board::removeZone(BI_Zone& zone) {
  if (mZones.value(zone.getData().getUuid()) != &zone) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mIsAddedToProject) {
    zone.removeFromBoard();  // can throw
  }
  mZones.remove(zone.getData().getUuid());
  emit zoneRemoved(zone);
}

/*******************************************************************************
 *  Polygon Methods
 ******************************************************************************/

void Board::addPolygon(BI_Polygon& polygon) {
  if ((mPolygons.values().contains(&polygon)) ||
      (&polygon.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mPolygons.contains(polygon.getData().getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a polygon with the UUID \"%1\"!")
            .arg(polygon.getData().getUuid().toStr()));
  }
  if (mIsAddedToProject) {
    polygon.addToBoard();  // can throw
  }
  mPolygons.insert(polygon.getData().getUuid(), &polygon);
  emit polygonAdded(polygon);
}

void Board::removePolygon(BI_Polygon& polygon) {
  if (mPolygons.value(polygon.getData().getUuid()) != &polygon) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mIsAddedToProject) {
    polygon.removeFromBoard();  // can throw
  }
  mPolygons.remove(polygon.getData().getUuid());
  emit polygonRemoved(polygon);
}

/*******************************************************************************
 *  StrokeText Methods
 ******************************************************************************/

void Board::addStrokeText(BI_StrokeText& text) {
  if ((mStrokeTexts.values().contains(&text)) || (&text.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mStrokeTexts.contains(text.getData().getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a stroke text with the UUID \"%1\"!")
            .arg(text.getData().getUuid().toStr()));
  }
  if (mIsAddedToProject) {
    text.addToBoard();  // can throw
  }
  mStrokeTexts.insert(text.getData().getUuid(), &text);
  emit strokeTextAdded(text);
}

void Board::removeStrokeText(BI_StrokeText& text) {
  if (mStrokeTexts.value(text.getData().getUuid()) != &text) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mIsAddedToProject) {
    text.removeFromBoard();  // can throw
  }
  mStrokeTexts.remove(text.getData().getUuid());
  emit strokeTextRemoved(text);
}

/*******************************************************************************
 *  Hole Methods
 ******************************************************************************/

void Board::addHole(BI_Hole& hole) {
  if ((mHoles.values().contains(&hole)) || (&hole.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mHoles.contains(hole.getData().getUuid())) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("There is already a hole with the UUID \"%1\"!")
                           .arg(hole.getData().getUuid().toStr()));
  }
  if (mIsAddedToProject) {
    hole.addToBoard();  // can throw
  }
  mHoles.insert(hole.getData().getUuid(), &hole);
  emit holeAdded(hole);
}

void Board::removeHole(BI_Hole& hole) {
  if (mHoles.value(hole.getData().getUuid()) != &hole) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mIsAddedToProject) {
    hole.removeFromBoard();  // can throw
  }
  mHoles.remove(hole.getData().getUuid());
  emit holeRemoved(hole);
}

/*******************************************************************************
 *  AirWire Methods
 ******************************************************************************/

void Board::triggerAirWiresRebuild() noexcept {
  if (!mIsAddedToProject) {
    return;
  }

  try {
    foreach (NetSignal* netsignal, mScheduledNetSignalsForAirWireRebuild) {
      // remove old airwires
      while (BI_AirWire* airWire = mAirWires.take(netsignal)) {
        airWire->removeFromBoard();  // can throw
        emit airWireRemoved(*airWire);
        delete airWire;
      }

      if (netsignal && netsignal->isAddedToCircuit()) {
        // calculate new airwires
        BoardAirWiresBuilder builder(*this, *netsignal);
        QVector<std::pair<const BI_NetLineAnchor*, const BI_NetLineAnchor*>>
            airwires = builder.buildAirWires();

        // add new airwires
        foreach (const auto& points, airwires) {
          std::unique_ptr<BI_AirWire> airWire(
              new BI_AirWire(*this, *netsignal, *points.first, *points.second));
          airWire->addToBoard();  // can throw
          mAirWires.insert(netsignal, airWire.get());
          emit airWireAdded(*airWire.release());
        }
      }
    }
    mScheduledNetSignalsForAirWireRebuild.clear();
  } catch (const std::exception&
               e) {  // std::exception because of the many std containers...
    qCritical() << "Failed to build airwires:" << e.what();
  }
}

void Board::forceAirWiresRebuild() noexcept {
  mScheduledNetSignalsForAirWireRebuild.unite(
      Toolbox::toSet(mProject.getCircuit().getNetSignals().values()));
  mScheduledNetSignalsForAirWireRebuild.unite(Toolbox::toSet(mAirWires.keys()));
  triggerAirWiresRebuild();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::optional<std::pair<Point, Point>> Board::calculateBoundingRect()
    const noexcept {
  QList<Path> outlines;
  foreach (const BI_Polygon* polygon, mPolygons) {
    if ((polygon->getData().getLayer() == Layer::boardOutlines()) &&
        (!polygon->getData().getPath().getVertices().isEmpty())) {
      outlines.append(polygon->getData().getPath());
    }
  }
  foreach (const BI_Device* device, mDeviceInstances) {
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if ((polygon.getLayer() == Layer::boardOutlines()) &&
          (!polygon.getPath().getVertices().isEmpty())) {
        outlines.append(Transform(*device).map(polygon.getPath()));
      }
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      if (circle.getLayer() == Layer::boardOutlines()) {
        outlines.append(Transform(*device).map(
            Path::circle(circle.getDiameter()).translated(circle.getCenter())));
      }
    }
  }
  if (!outlines.isEmpty()) {
    QPainterPath p;
    foreach (const Path& outline, outlines) {
      p.addPath(outline.toQPainterPathPx());
    }
    const QRectF rectPx = p.boundingRect();
    const Point bottomLeft = Point::fromPx(rectPx.bottomLeft());
    const Point topRight = Point::fromPx(rectPx.topRight());
    return std::make_pair(bottomLeft, topRight);
  } else {
    return std::nullopt;
  }
}

void Board::addDefaultContent() {
  // Add 100x80mm board outline (1/2 Eurocard size).
  addPolygon(*new BI_Polygon(
      *this,
      BoardPolygonData(Uuid::createRandom(), Layer::boardOutlines(),
                       UnsignedLength(0),
                       Path::rect(Point(0, 0), Point(100000000, 80000000)),
                       false, false, false)));
}

void Board::copyFrom(const Board& other) {
  mDefaultFontFileName = other.getDefaultFontName();
  mGridInterval = other.getGridInterval();
  mGridUnit = other.getGridUnit();
  mInnerLayerCount = other.getInnerLayerCount();
  mCopperLayers = other.getCopperLayers();
  mPcbThickness = other.mPcbThickness;
  mSolderResist = other.mSolderResist;
  mSilkscreenColor = other.mSilkscreenColor;
  mSilkscreenLayersTop = other.mSilkscreenLayersTop;
  mSilkscreenLayersBot = other.mSilkscreenLayersBot;
  *mDesignRules = other.getDesignRules();
  *mDrcSettings = other.getDrcSettings();
  *mFabricationOutputSettings = other.getFabricationOutputSettings();

  // Copy device instances.
  QHash<const BI_Device*, BI_Device*> devMap;
  foreach (const BI_Device* device, other.getDeviceInstances()) {
    BI_Device* copy = new BI_Device(
        *this, device->getComponentInstance(), device->getLibDevice().getUuid(),
        device->getLibFootprint().getUuid(), device->getPosition(),
        device->getRotation(), device->getMirrored(), device->isLocked(),
        device->isGlueEnabled(), false);
    copy->setAttributes(device->getAttributes());
    if (const PackageModel* model = device->getLibModel()) {
      copy->setModel(model->getUuid());
    }
    foreach (const BI_StrokeText* text, device->getStrokeTexts()) {
      copy->addStrokeText(*new BI_StrokeText(*this, text->getData()));
    }
    addDeviceInstance(*copy);
    devMap.insert(device, copy);
  }

  // Copy netsegments.
  foreach (const BI_NetSegment* netSegment, other.getNetSegments()) {
    BI_NetSegment* copy = new BI_NetSegment(*this, Uuid::createRandom(),
                                            netSegment->getNetSignal());

    // Determine new pad anchors.
    QHash<const BI_NetLineAnchor*, BI_NetLineAnchor*> anchorsMap;
    for (auto it = devMap.begin(); it != devMap.end(); ++it) {
      const BI_Device& oldDev = *it.key();
      BI_Device& newDev = *it.value();
      foreach (const BI_FootprintPad* pad, oldDev.getPads()) {
        anchorsMap.insert(pad, newDev.getPad(pad->getLibPadUuid()));
      }
    }

    // Copy vias.
    QList<BI_Via*> vias;
    foreach (const BI_Via* via, netSegment->getVias()) {
      BI_Via* viaCopy =
          new BI_Via(*copy, Via(Uuid::createRandom(), via->getVia()));
      vias.append(viaCopy);
      anchorsMap.insert(via, viaCopy);
    }

    // Copy netpoints.
    QList<BI_NetPoint*> netPoints;
    foreach (const BI_NetPoint* netPoint, netSegment->getNetPoints()) {
      BI_NetPoint* netPointCopy =
          new BI_NetPoint(*copy, Uuid::createRandom(), netPoint->getPosition());
      netPoints.append(netPointCopy);
      anchorsMap.insert(netPoint, netPointCopy);
    }

    // Copy netlines.
    QList<BI_NetLine*> netLines;
    foreach (const BI_NetLine* netLine, netSegment->getNetLines()) {
      BI_NetLineAnchor* p1 = anchorsMap.value(&netLine->getP1());
      Q_ASSERT(p1);
      BI_NetLineAnchor* p2 = anchorsMap.value(&netLine->getP2());
      Q_ASSERT(p2);
      BI_NetLine* netLineCopy =
          new BI_NetLine(*copy, Uuid::createRandom(), *p1, *p2,
                         netLine->getLayer(), netLine->getWidth());
      netLines.append(netLineCopy);
    }

    copy->addElements(vias, netPoints, netLines);
    addNetSegment(*copy);
  }

  // Copy planes.
  foreach (const BI_Plane* plane, other.getPlanes()) {
    BI_Plane* copy =
        new BI_Plane(*this, Uuid::createRandom(), plane->getLayer(),
                     plane->getNetSignal(), plane->getOutline());
    copy->setMinWidth(plane->getMinWidth());
    copy->setMinClearance(plane->getMinClearance());
    copy->setKeepIslands(plane->getKeepIslands());
    copy->setPriority(plane->getPriority());
    copy->setConnectStyle(plane->getConnectStyle());
    copy->setThermalGap(plane->getThermalGap());
    copy->setThermalSpokeWidth(plane->getThermalSpokeWidth());
    copy->setLocked(plane->isLocked());
    copy->setVisible(plane->isVisible());
    copy->setCalculatedFragments(plane->getFragments());
    addPlane(*copy);
  }

  // Copy zones.
  foreach (const BI_Zone* zone, other.getZones()) {
    BI_Zone* copy = new BI_Zone(
        *this, BoardZoneData(Uuid::createRandom(), zone->getData()));
    addZone(*copy);
  }

  // Copy polygons.
  foreach (const BI_Polygon* polygon, other.getPolygons()) {
    BI_Polygon* copy = new BI_Polygon(
        *this, BoardPolygonData(Uuid::createRandom(), polygon->getData()));
    addPolygon(*copy);
  }

  // Copy stroke texts.
  foreach (const BI_StrokeText* text, other.getStrokeTexts()) {
    BI_StrokeText* copy = new BI_StrokeText(
        *this, BoardStrokeTextData(Uuid::createRandom(), text->getData()));
    addStrokeText(*copy);
  }

  // Copy holes.
  foreach (const BI_Hole* hole, other.getHoles()) {
    BI_Hole* copy = new BI_Hole(
        *this, BoardHoleData{Uuid::createRandom(), hole->getData()});
    addHole(*copy);
  }

  invalidatePlanes();
}

void Board::addToProject() {
  if (mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }

  QList<BI_Base*> items = getAllItems();
  ScopeGuardList sgl(items.count());
  for (int i = 0; i < items.count(); ++i) {
    BI_Base* item = items.at(i);
    item->addToBoard();  // can throw
    sgl.add([item]() { item->removeFromBoard(); });
  }

  // Move directory atomically (last step which could throw an exception).
  if (mDirectory->getFileSystem() != mProject.getDirectory().getFileSystem()) {
    TransactionalDirectory dst(mProject.getDirectory(),
                               "boards/" % mDirectoryName);
    mDirectory->moveTo(dst);  // can throw
  }

  mIsAddedToProject = true;
  forceAirWiresRebuild();
  sgl.dismiss();
}

void Board::removeFromProject() {
  if (!mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }

  QList<BI_Base*> items = getAllItems();
  ScopeGuardList sgl(items.count());
  for (int i = items.count() - 1; i >= 0; --i) {
    BI_Base* item = items.at(i);
    item->removeFromBoard();  // can throw
    sgl.add([item]() { item->addToBoard(); });
  }

  // Move directory atomically (last step which could throw an exception).
  TransactionalDirectory tmp;
  mDirectory->moveTo(tmp);  // can throw

  mIsAddedToProject = false;
  sgl.dismiss();
}

void Board::save() {
  // Content.
  {
    std::unique_ptr<SExpression> root =
        SExpression::createList("librepcb_board");
    root->appendChild(mUuid);
    root->ensureLineBreak();
    root->appendChild("name", mName);
    root->ensureLineBreak();
    root->appendChild("default_font", mDefaultFontFileName);
    root->ensureLineBreak();
    SExpression& gridNode = root->appendList("grid");
    gridNode.appendChild("interval", mGridInterval);
    gridNode.appendChild("unit", mGridUnit);
    root->ensureLineBreak();
    {
      SExpression& node = root->appendList("layers");
      node.appendChild("inner", mInnerLayerCount);
    }
    root->ensureLineBreak();
    root->appendChild("thickness", mPcbThickness);
    root->ensureLineBreak();
    root->appendChild("solder_resist", mSolderResist);
    root->ensureLineBreak();
    root->appendChild("silkscreen", mSilkscreenColor);
    root->ensureLineBreak();
    {
      SExpression& node = root->appendList("silkscreen_layers_top");
      foreach (const Layer* layer, mSilkscreenLayersTop) {
        node.appendChild(*layer);
      }
    }
    root->ensureLineBreak();
    {
      SExpression& node = root->appendList("silkscreen_layers_bot");
      foreach (const Layer* layer, mSilkscreenLayersBot) {
        node.appendChild(*layer);
      }
    }
    root->ensureLineBreak();
    mDesignRules->serialize(root->appendList("design_rules"));
    root->ensureLineBreak();
    {
      SExpression& node = root->appendList("design_rule_check");
      mDrcSettings->serialize(node);
      node.appendChild("approvals_version", mDrcMessageApprovalsVersion);
      node.ensureLineBreak();
      foreach (const SExpression& child,
               Toolbox::sortedQSet(mDrcMessageApprovals)) {
        node.appendChild(child);
        node.ensureLineBreak();
      }
    }
    root->ensureLineBreak();
    mFabricationOutputSettings->serialize(
        root->appendList("fabrication_output_settings"));
    root->ensureLineBreak();
    for (const BI_Device* obj : mDeviceInstances) {
      root->ensureLineBreak();
      obj->serialize(root->appendList("device"));
    }
    root->ensureLineBreak();
    for (const BI_NetSegment* obj : mNetSegments) {
      root->ensureLineBreak();
      obj->serialize(root->appendList("netsegment"));
    }
    root->ensureLineBreak();
    for (const BI_Plane* obj : mPlanes) {
      root->ensureLineBreak();
      obj->serialize(root->appendList("plane"));
    }
    for (const BI_Zone* obj : mZones) {
      root->ensureLineBreak();
      obj->getData().serialize(root->appendList("zone"));
    }
    root->ensureLineBreak();
    for (const BI_Polygon* obj : mPolygons) {
      root->ensureLineBreak();
      obj->getData().serialize(root->appendList("polygon"));
    }
    root->ensureLineBreak();
    for (const BI_StrokeText* obj : mStrokeTexts) {
      root->ensureLineBreak();
      obj->getData().serialize(root->appendList("stroke_text"));
    }
    root->ensureLineBreak();
    for (const BI_Hole* obj : mHoles) {
      root->ensureLineBreak();
      obj->getData().serialize(root->appendList("hole"));
    }
    root->ensureLineBreak();
    mDirectory->write("board.lp", root->toByteArray());
  }

  // User settings.
  {
    std::unique_ptr<SExpression> root =
        SExpression::createList("librepcb_board_user_settings");
    for (auto it = mLayersVisibility.begin(); it != mLayersVisibility.end();
         it++) {
      root->ensureLineBreak();
      SExpression& child = root->appendList("layer");
      child.appendChild(SExpression::createToken(it.key()));
      child.appendChild("visible", it.value());
    }
    root->ensureLineBreak();
    for (const BI_Plane* plane : mPlanes) {
      root->ensureLineBreak();
      SExpression& node = root->appendList("plane");
      node.appendChild(plane->getUuid());
      node.appendChild("visible", plane->isVisible());
    }
    root->ensureLineBreak();
    mDirectory->write("settings.user.lp", root->toByteArray());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
