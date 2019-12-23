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

#include "../circuit/circuit.h"
#include "../circuit/componentinstance.h"
#include "../circuit/netsignal.h"
#include "../erc/ercmsg.h"
#include "../project.h"
#include "boardairwiresbuilder.h"
#include "boardfabricationoutputsettings.h"
#include "boardlayerstack.h"
#include "boardselectionquery.h"
#include "boardusersettings.h"
#include "items/bi_airwire.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_via.h"

#include <librepcb/common/application.h>
#include <librepcb/common/boarddesignrules.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/scopeguardlist.h>
#include <librepcb/common/toolbox.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/pkg/footprint.h>

#include <QtCore>
#include <QtWidgets>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Board::Board(const Board&                            other,
             std::unique_ptr<TransactionalDirectory> directory,
             const ElementName&                      name)
  : QObject(&other.getProject()),
    mProject(other.getProject()),
    mDirectory(std::move(directory)),
    mIsAddedToProject(false),
    mUuid(Uuid::createRandom()),
    mName(name),
    mDefaultFontFileName(other.mDefaultFontFileName) {
  try {
    mGraphicsScene.reset(new GraphicsScene());

    // copy layer stack
    mLayerStack.reset(new BoardLayerStack(*this, *other.mLayerStack));

    // copy grid properties
    mGridProperties.reset(new GridProperties(*other.mGridProperties));

    // copy design rules
    mDesignRules.reset(new BoardDesignRules(*other.mDesignRules));

    // copy fabrication output settings
    mFabricationOutputSettings.reset(
        new BoardFabricationOutputSettings(*other.mFabricationOutputSettings));

    // copy user settings
    mUserSettings.reset(new BoardUserSettings(*this, *other.mUserSettings));

    // copy device instances
    QHash<const BI_Device*, BI_Device*> copiedDeviceInstances;
    foreach (const BI_Device* device, other.mDeviceInstances) {
      BI_Device* copy = new BI_Device(*this, *device);
      Q_ASSERT(
          !getDeviceInstanceByComponentUuid(copy->getComponentInstanceUuid()));
      mDeviceInstances.insert(copy->getComponentInstanceUuid(), copy);
      copiedDeviceInstances.insert(device, copy);
    }

    // copy netsegments
    foreach (const BI_NetSegment* netsegment, other.mNetSegments) {
      BI_NetSegment* copy =
          new BI_NetSegment(*this, *netsegment, copiedDeviceInstances);
      Q_ASSERT(!getNetSegmentByUuid(copy->getUuid()));
      mNetSegments.append(copy);
    }

    // copy planes
    foreach (const BI_Plane* plane, other.mPlanes) {
      BI_Plane* copy = new BI_Plane(*this, *plane);
      mPlanes.append(copy);
    }

    // copy polygons
    foreach (const BI_Polygon* polygon, other.mPolygons) {
      BI_Polygon* copy = new BI_Polygon(*this, *polygon);
      mPolygons.append(copy);
    }

    // copy stroke texts
    foreach (const BI_StrokeText* text, other.mStrokeTexts) {
      BI_StrokeText* copy = new BI_StrokeText(*this, *text);
      mStrokeTexts.append(copy);
    }

    // copy holes
    foreach (const BI_Hole* hole, other.mHoles) {
      BI_Hole* copy = new BI_Hole(*this, *hole);
      mHoles.append(copy);
    }

    // rebuildAllPlanes(); --> fragments are copied too, so no need to rebuild
    // them
    updateErcMessages();
    updateIcon();

    // emit the "attributesChanged" signal when the project has emited it
    connect(&mProject, &Project::attributesChanged, this,
            &Board::attributesChanged);

    connect(&mProject.getCircuit(), &Circuit::componentAdded, this,
            &Board::updateErcMessages);
    connect(&mProject.getCircuit(), &Circuit::componentRemoved, this,
            &Board::updateErcMessages);
  } catch (...) {
    // free the allocated memory in the reverse order of their allocation...
    qDeleteAll(mErcMsgListUnplacedComponentInstances);
    mErcMsgListUnplacedComponentInstances.clear();
    qDeleteAll(mAirWires);
    mAirWires.clear();
    qDeleteAll(mHoles);
    mHoles.clear();
    qDeleteAll(mStrokeTexts);
    mStrokeTexts.clear();
    qDeleteAll(mPolygons);
    mPolygons.clear();
    qDeleteAll(mPlanes);
    mPlanes.clear();
    qDeleteAll(mNetSegments);
    mNetSegments.clear();
    qDeleteAll(mDeviceInstances);
    mDeviceInstances.clear();
    mUserSettings.reset();
    mFabricationOutputSettings.reset();
    mDesignRules.reset();
    mGridProperties.reset();
    mLayerStack.reset();
    mGraphicsScene.reset();
    throw;  // ...and rethrow the exception
  }
}

Board::Board(Project&                                project,
             std::unique_ptr<TransactionalDirectory> directory, bool create,
             const QString& newName)
  : QObject(&project),
    mProject(project),
    mDirectory(std::move(directory)),
    mIsAddedToProject(false),
    mUuid(Uuid::createRandom()),
    mName("New Board") {
  try {
    mGraphicsScene.reset(new GraphicsScene());

    // try to open/create the board file
    if (create) {
      // set attributes
      mName                = ElementName(newName);  // can throw
      mDefaultFontFileName = qApp->getDefaultStrokeFontName();

      // load default layer stack
      mLayerStack.reset(new BoardLayerStack(*this));

      // load default grid properties (smaller grid than in schematics to avoid
      // grid snap issues)
      mGridProperties.reset(new GridProperties(GridProperties::Type_t::Lines,
                                               PositiveLength(635000),
                                               LengthUnit::millimeters()));

      // load default design rules
      mDesignRules.reset(new BoardDesignRules());

      // load default fabrication output settings
      mFabricationOutputSettings.reset(new BoardFabricationOutputSettings());

      // load default user settings
      mUserSettings.reset(new BoardUserSettings(*this));

      // add 100x80mm board outline (1/2 Eurocard size)
      Polygon polygon(Uuid::createRandom(),
                      GraphicsLayerName(GraphicsLayer::sBoardOutlines),
                      UnsignedLength(0), false, false,
                      Path::rect(Point(0, 0), Point(100000000, 80000000)));
      mPolygons.append(new BI_Polygon(*this, polygon));
    } else {
      SExpression root = SExpression::parse(
          mDirectory->read(getFilePath().getFilename()), getFilePath());

      // the board seems to be ready to open, so we will create all needed
      // objects

      mUuid = root.getChildByIndex(0).getValue<Uuid>();
      mName = root.getValueByPath<ElementName>("name");
      if (const SExpression* child = root.tryGetChildByPath("default_font")) {
        mDefaultFontFileName = child->getValueOfFirstChild<QString>(true);
      } else {
        mDefaultFontFileName = qApp->getDefaultStrokeFontName();
      }

      // Load grid properties
      mGridProperties.reset(new GridProperties(root.getChildByPath("grid")));

      // Load layer stack
      mLayerStack.reset(
          new BoardLayerStack(*this, root.getChildByPath("layers")));

      // load design rules
      mDesignRules.reset(
          new BoardDesignRules(root.getChildByPath("design_rules")));

      // load fabrication output settings
      mFabricationOutputSettings.reset(new BoardFabricationOutputSettings(
          root.getChildByPath("fabrication_output_settings")));

      // load user settings
      try {
        QString     userSettingsFp = "settings.user.lp";
        SExpression userSettingsRoot =
            SExpression::parse(mDirectory->read(userSettingsFp),
                               mDirectory->getAbsPath(userSettingsFp));
        mUserSettings.reset(new BoardUserSettings(*this, userSettingsRoot));
      } catch (const Exception&) {
        // Project user settings are normally not put under version control and
        // thus the likelyhood of parse errors is higher (e.g. when switching to
        // an older, now incompatible revision). To avoid frustration, we just
        // ignore these errors and load the default settings instead...
        qCritical() << "Could not open board user settings, defaults will be "
                       "used instead!";
        mUserSettings.reset(new BoardUserSettings(*this));
      }

      // Load all device instances
      foreach (const SExpression& node, root.getChildren("device")) {
        BI_Device* device = new BI_Device(*this, node);
        if (getDeviceInstanceByComponentUuid(
                device->getComponentInstanceUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString(tr("There is already a device of the component instance "
                         "\"%1\"!"))
                  .arg(device->getComponentInstanceUuid().toStr()));
        }
        mDeviceInstances.insert(device->getComponentInstanceUuid(), device);
      }

      // Load all netsegments
      foreach (const SExpression& node, root.getChildren("netsegment")) {
        BI_NetSegment* netsegment = new BI_NetSegment(*this, node);
        if (getNetSegmentByUuid(netsegment->getUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString(tr("There is already a netsegment with the UUID \"%1\"!"))
                  .arg(netsegment->getUuid().toStr()));
        }
        mNetSegments.append(netsegment);
      }

      // Load all planes
      foreach (const SExpression& node, root.getChildren("plane")) {
        BI_Plane* plane = new BI_Plane(*this, node);
        mPlanes.append(plane);
      }

      // Load all polygons
      foreach (const SExpression& node, root.getChildren("polygon")) {
        BI_Polygon* polygon = new BI_Polygon(*this, node);
        mPolygons.append(polygon);
      }

      // Load all stroke texts
      foreach (const SExpression& node, root.getChildren("stroke_text")) {
        BI_StrokeText* text = new BI_StrokeText(*this, node);
        mStrokeTexts.append(text);
      }

      // Load all holes
      foreach (const SExpression& node, root.getChildren("hole")) {
        BI_Hole* hole = new BI_Hole(*this, node);
        mHoles.append(hole);
      }
    }

    rebuildAllPlanes();
    updateErcMessages();
    updateIcon();

    // emit the "attributesChanged" signal when the project has emited it
    connect(&mProject, &Project::attributesChanged, this,
            &Board::attributesChanged);

    connect(&mProject.getCircuit(), &Circuit::componentAdded, this,
            &Board::updateErcMessages);
    connect(&mProject.getCircuit(), &Circuit::componentRemoved, this,
            &Board::updateErcMessages);
  } catch (...) {
    // free the allocated memory in the reverse order of their allocation...
    qDeleteAll(mErcMsgListUnplacedComponentInstances);
    mErcMsgListUnplacedComponentInstances.clear();
    qDeleteAll(mAirWires);
    mAirWires.clear();
    qDeleteAll(mHoles);
    mHoles.clear();
    qDeleteAll(mStrokeTexts);
    mStrokeTexts.clear();
    qDeleteAll(mPolygons);
    mPolygons.clear();
    qDeleteAll(mPlanes);
    mPlanes.clear();
    qDeleteAll(mNetSegments);
    mNetSegments.clear();
    qDeleteAll(mDeviceInstances);
    mDeviceInstances.clear();
    mUserSettings.reset();
    mFabricationOutputSettings.reset();
    mDesignRules.reset();
    mGridProperties.reset();
    mLayerStack.reset();
    mGraphicsScene.reset();
    throw;  // ...and rethrow the exception
  }
}

Board::~Board() noexcept {
  Q_ASSERT(!mIsAddedToProject);

  qDeleteAll(mErcMsgListUnplacedComponentInstances);
  mErcMsgListUnplacedComponentInstances.clear();

  // delete all items
  qDeleteAll(mAirWires);
  mAirWires.clear();
  qDeleteAll(mHoles);
  mHoles.clear();
  qDeleteAll(mStrokeTexts);
  mStrokeTexts.clear();
  qDeleteAll(mPolygons);
  mPolygons.clear();
  qDeleteAll(mPlanes);
  mPlanes.clear();
  qDeleteAll(mNetSegments);
  mNetSegments.clear();
  qDeleteAll(mDeviceInstances);
  mDeviceInstances.clear();

  mUserSettings.reset();
  mFabricationOutputSettings.reset();
  mDesignRules.reset();
  mGridProperties.reset();
  mLayerStack.reset();
  mGraphicsScene.reset();
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

FilePath Board::getFilePath() const noexcept {
  return mDirectory->getAbsPath("board.lp");
}

bool Board::isEmpty() const noexcept {
  return (mDeviceInstances.isEmpty() && mNetSegments.isEmpty() &&
          mPlanes.isEmpty() && mPolygons.isEmpty() && mStrokeTexts.isEmpty() &&
          mHoles.isEmpty());
}

QList<BI_Base*> Board::getItemsAtScenePos(const Point& pos) const noexcept {
  QPointF scenePosPx = pos.toPxQPointF();
  QList<BI_Base*>
      list;  // Note: The order of adding the items is very important (the
             // top most item must appear as the first item in the list)!
  // vias
  foreach (BI_Via* via, getViasAtScenePos(pos, nullptr)) { list.append(via); }
  // netpoints
  foreach (BI_NetPoint* netpoint,
           getNetPointsAtScenePos(pos, nullptr, nullptr)) {
    list.append(netpoint);
  }
  // netlines
  foreach (BI_NetLine* netline, getNetLinesAtScenePos(pos, nullptr, nullptr)) {
    list.append(netline);
  }
  // footprints & pads
  foreach (BI_Device* device, mDeviceInstances) {
    BI_Footprint& footprint = device->getFootprint();
    if (footprint.isSelectable() &&
        footprint.getGrabAreaScenePx().contains(scenePosPx)) {
      if (footprint.getIsMirrored()) {
        list.append(&footprint);
      } else {
        list.prepend(&footprint);
      }
    }
    foreach (BI_FootprintPad* pad, footprint.getPads()) {
      if (pad->isSelectable() &&
          pad->getGrabAreaScenePx().contains(scenePosPx)) {
        if (pad->getIsMirrored()) {
          list.append(pad);
        } else {
          list.insert(1, pad);
        }
      }
    }
    foreach (BI_StrokeText* text, device->getFootprint().getStrokeTexts()) {
      if (text->isSelectable() &&
          text->getGrabAreaScenePx().contains(scenePosPx)) {
        if (GraphicsLayer::isTopLayer(*text->getText().getLayerName())) {
          list.prepend(text);
        } else {
          list.append(text);
        }
      }
    }
  }
  // planes
  foreach (BI_Plane* planes, mPlanes) {
    if (planes->isSelectable() &&
        planes->getGrabAreaScenePx().contains(scenePosPx)) {
      list.append(planes);
    }
  }
  // polygons
  foreach (BI_Polygon* polygon, mPolygons) {
    if (polygon->isSelectable() &&
        polygon->getGrabAreaScenePx().contains(scenePosPx)) {
      list.append(polygon);
    }
  }
  // texts
  foreach (BI_StrokeText* text, mStrokeTexts) {
    if (text->isSelectable() &&
        text->getGrabAreaScenePx().contains(scenePosPx)) {
      list.append(text);
    }
  }
  // holes
  foreach (BI_Hole* hole, mHoles) {
    if (hole->isSelectable() &&
        hole->getGrabAreaScenePx().contains(scenePosPx)) {
      list.append(hole);
    }
  }
  return list;
}

QList<BI_Via*> Board::getViasAtScenePos(const Point&     pos,
                                        const NetSignal* netsignal) const
    noexcept {
  QList<BI_Via*> list;
  foreach (BI_NetSegment* segment, mNetSegments) {
    if ((!netsignal) || (&segment->getNetSignal() == netsignal)) {
      segment->getViasAtScenePos(pos, list);
    }
  }
  return list;
}

QList<BI_NetPoint*> Board::getNetPointsAtScenePos(
    const Point& pos, const GraphicsLayer* layer,
    const NetSignal* netsignal) const noexcept {
  QList<BI_NetPoint*> list;
  foreach (BI_NetSegment* segment, mNetSegments) {
    if ((!netsignal) || (&segment->getNetSignal() == netsignal)) {
      segment->getNetPointsAtScenePos(pos, layer, list);
    }
  }
  return list;
}

QList<BI_NetLine*> Board::getNetLinesAtScenePos(
    const Point& pos, const GraphicsLayer* layer,
    const NetSignal* netsignal) const noexcept {
  QList<BI_NetLine*> list;
  foreach (BI_NetSegment* segment, mNetSegments) {
    if ((!netsignal) || (&segment->getNetSignal() == netsignal)) {
      segment->getNetLinesAtScenePos(pos, layer, list);
    }
  }
  return list;
}

QList<BI_FootprintPad*> Board::getPadsAtScenePos(
    const Point& pos, const GraphicsLayer* layer,
    const NetSignal* netsignal) const noexcept {
  QList<BI_FootprintPad*> list;
  foreach (BI_Device* device, mDeviceInstances) {
    foreach (BI_FootprintPad* pad, device->getFootprint().getPads()) {
      if (pad->isSelectable() &&
          pad->getGrabAreaScenePx().contains(pos.toPxQPointF()) &&
          ((!layer) || (pad->isOnLayer(layer->getName()))) &&
          ((!netsignal) || (pad->getCompSigInstNetSignal() == netsignal))) {
        list.append(pad);
      }
    }
  }
  return list;
}

QList<BI_Base*> Board::getAllItems() const noexcept {
  QList<BI_Base*> items;
  foreach (BI_Device* device, mDeviceInstances)
    items.append(device);
  foreach (BI_NetSegment* netsegment, mNetSegments)
    items.append(netsegment);
  foreach (BI_Plane* plane, mPlanes)
    items.append(plane);
  foreach (BI_Polygon* polygon, mPolygons)
    items.append(polygon);
  foreach (BI_StrokeText* text, mStrokeTexts)
    items.append(text);
  foreach (BI_Hole* hole, mHoles)
    items.append(hole);
  foreach (BI_AirWire* airWire, mAirWires)
    items.append(airWire);
  return items;
}

/*******************************************************************************
 *  Setters: General
 ******************************************************************************/

void Board::setGridProperties(const GridProperties& grid) noexcept {
  *mGridProperties = grid;
}

/*******************************************************************************
 *  DeviceInstance Methods
 ******************************************************************************/

BI_Device* Board::getDeviceInstanceByComponentUuid(const Uuid& uuid) const
    noexcept {
  return mDeviceInstances.value(uuid, nullptr);
}

void Board::addDeviceInstance(BI_Device& instance) {
  if ((!mIsAddedToProject) || (&instance.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  // check if there is no device with the same component instance in the list
  if (getDeviceInstanceByComponentUuid(
          instance.getComponentInstance().getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(
            tr("There is already a device with the component instance \"%1\"!"))
            .arg(instance.getComponentInstance().getUuid().toStr()));
  }
  // add to board
  instance.addToBoard();  // can throw
  mDeviceInstances.insert(instance.getComponentInstanceUuid(), &instance);
  updateErcMessages();
  emit deviceAdded(instance);
}

void Board::removeDeviceInstance(BI_Device& instance) {
  if ((!mIsAddedToProject) ||
      (!mDeviceInstances.contains(instance.getComponentInstanceUuid()))) {
    throw LogicError(__FILE__, __LINE__);
  }
  // remove from board
  instance.removeFromBoard();  // can throw
  mDeviceInstances.remove(instance.getComponentInstanceUuid());
  updateErcMessages();
  emit deviceRemoved(instance);
}

/*******************************************************************************
 *  NetSegment Methods
 ******************************************************************************/

BI_NetSegment* Board::getNetSegmentByUuid(const Uuid& uuid) const noexcept {
  foreach (BI_NetSegment* netsegment, mNetSegments) {
    if (netsegment->getUuid() == uuid) return netsegment;
  }
  return nullptr;
}

void Board::addNetSegment(BI_NetSegment& netsegment) {
  if ((!mIsAddedToProject) || (mNetSegments.contains(&netsegment)) ||
      (&netsegment.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  // check if there is no netsegment with the same uuid in the list
  if (getNetSegmentByUuid(netsegment.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("There is already a netsegment with the UUID \"%1\"!"))
            .arg(netsegment.getUuid().toStr()));
  }
  // add to board
  netsegment.addToBoard();  // can throw
  mNetSegments.append(&netsegment);
}

void Board::removeNetSegment(BI_NetSegment& netsegment) {
  if ((!mIsAddedToProject) || (!mNetSegments.contains(&netsegment))) {
    throw LogicError(__FILE__, __LINE__);
  }
  // remove from board
  netsegment.removeFromBoard();  // can throw
  mNetSegments.removeOne(&netsegment);
}

/*******************************************************************************
 *  Plane Methods
 ******************************************************************************/

void Board::addPlane(BI_Plane& plane) {
  if ((!mIsAddedToProject) || (mPlanes.contains(&plane)) ||
      (&plane.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  plane.addToBoard();  // can throw
  mPlanes.append(&plane);
}

void Board::removePlane(BI_Plane& plane) {
  if ((!mIsAddedToProject) || (!mPlanes.contains(&plane))) {
    throw LogicError(__FILE__, __LINE__);
  }
  plane.removeFromBoard();  // can throw
  mPlanes.removeOne(&plane);
}

void Board::rebuildAllPlanes() noexcept {
  QList<BI_Plane*> planes = mPlanes;
  std::sort(planes.begin(), planes.end(),
            [](const BI_Plane* p1, const BI_Plane* p2) {
              return !(*p1 < *p2);
            });  // sort by priority (highest priority first)
  foreach (BI_Plane* plane, planes) { plane->rebuild(); }
}

/*******************************************************************************
 *  Polygon Methods
 ******************************************************************************/

void Board::addPolygon(BI_Polygon& polygon) {
  if ((!mIsAddedToProject) || (mPolygons.contains(&polygon)) ||
      (&polygon.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  polygon.addToBoard();  // can throw
  mPolygons.append(&polygon);
}

void Board::removePolygon(BI_Polygon& polygon) {
  if ((!mIsAddedToProject) || (!mPolygons.contains(&polygon))) {
    throw LogicError(__FILE__, __LINE__);
  }
  polygon.removeFromBoard();  // can throw
  mPolygons.removeOne(&polygon);
}

/*******************************************************************************
 *  StrokeText Methods
 ******************************************************************************/

void Board::addStrokeText(BI_StrokeText& text) {
  if ((!mIsAddedToProject) || (mStrokeTexts.contains(&text)) ||
      (&text.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  text.addToBoard();  // can throw
  mStrokeTexts.append(&text);
}

void Board::removeStrokeText(BI_StrokeText& text) {
  if ((!mIsAddedToProject) || (!mStrokeTexts.contains(&text))) {
    throw LogicError(__FILE__, __LINE__);
  }
  text.removeFromBoard();  // can throw
  mStrokeTexts.removeOne(&text);
}

/*******************************************************************************
 *  Hole Methods
 ******************************************************************************/

void Board::addHole(BI_Hole& hole) {
  if ((!mIsAddedToProject) || (mHoles.contains(&hole)) ||
      (&hole.getBoard() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  hole.addToBoard();  // can throw
  mHoles.append(&hole);
}

void Board::removeHole(BI_Hole& hole) {
  if ((!mIsAddedToProject) || (!mHoles.contains(&hole))) {
    throw LogicError(__FILE__, __LINE__);
  }
  hole.removeFromBoard();  // can throw
  mHoles.removeOne(&hole);
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
        delete airWire;
      }

      if (netsignal && netsignal->isAddedToCircuit()) {
        // calculate new airwires
        BoardAirWiresBuilder         builder(*this, *netsignal);
        QVector<QPair<Point, Point>> airwires = builder.buildAirWires();

        // add new airwires
        foreach (const auto& points, airwires) {
          QScopedPointer<BI_AirWire> airWire(
              new BI_AirWire(*this, *netsignal, points.first, points.second));
          airWire->addToBoard();  // can throw
          mAirWires.insertMulti(netsignal, airWire.take());
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

void Board::addToProject() {
  if (mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }
  QList<BI_Base*> items = getAllItems();
  ScopeGuardList  sgl(items.count());
  for (int i = 0; i < items.count(); ++i) {
    BI_Base* item = items.at(i);
    item->addToBoard();  // can throw
    sgl.add([item]() { item->removeFromBoard(); });
  }
  mIsAddedToProject = true;
  forceAirWiresRebuild();
  updateErcMessages();
  sgl.dismiss();
}

void Board::removeFromProject() {
  if (!mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }
  QList<BI_Base*> items = getAllItems();
  ScopeGuardList  sgl(items.count());
  for (int i = items.count() - 1; i >= 0; --i) {
    BI_Base* item = items.at(i);
    item->removeFromBoard();  // can throw
    sgl.add([item]() { item->addToBoard(); });
  }
  mIsAddedToProject = false;
  updateErcMessages();
  sgl.dismiss();
}

void Board::save() {
  if (mIsAddedToProject) {
    // save board file
    SExpression brdDoc(serializeToDomElement("librepcb_board"));  // can throw
    mDirectory->write(getFilePath().getFilename(),
                      brdDoc.toByteArray());  // can throw

    // save user settings
    SExpression usrDoc(mUserSettings->serializeToDomElement(
        "librepcb_board_user_settings"));                         // can throw
    mDirectory->write("settings.user.lp", usrDoc.toByteArray());  // can throw
  } else {
    mDirectory->removeDirRecursively();  // can throw
  }
}

void Board::print(QPrinter& printer) {
  clearSelection();

  // Adjust layer colors
  ScopeGuardList sgl;
  foreach (GraphicsLayer* layer, mLayerStack->getAllLayers()) {
    QColor color = layer->getColor();
    sgl.add([layer, color]() { layer->setColor(color); });  // restore color
    int h = color.hsvHue();
    int s = color.hsvSaturation();
    int v = color.value() / 2;          // avoid white colors
    int a = (color.alpha() / 2) + 127;  // avoid transparent colors
    layer->setColor(QColor::fromHsv(h, s, v, a));
  }

  QPainter painter(&printer);
  renderToQPainter(painter, printer.resolution());  // can throw
}

void Board::renderToQPainter(QPainter& painter, int dpi) const {
  QRectF sceneRect = mGraphicsScene->itemsBoundingRect();
  QRectF printerRect(
      qreal(0), qreal(0),
      Length::fromPx(sceneRect.width()).toInch() * dpi,    // can throw
      Length::fromPx(sceneRect.height()).toInch() * dpi);  // can throw
  mGraphicsScene->render(&painter, printerRect, sceneRect,
                         Qt::IgnoreAspectRatio);
}

void Board::showInView(GraphicsView& view) noexcept {
  view.setScene(mGraphicsScene.data());
}

void Board::setSelectionRect(const Point& p1, const Point& p2,
                             bool updateItems) noexcept {
  mGraphicsScene->setSelectionRect(p1, p2);
  if (updateItems) {
    QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
    foreach (BI_Device* component, mDeviceInstances) {
      BI_Footprint& footprint       = component->getFootprint();
      bool          selectFootprint = footprint.isSelectable() &&
                             footprint.getGrabAreaScenePx().intersects(rectPx);
      footprint.setSelected(selectFootprint);
      foreach (BI_FootprintPad* pad, footprint.getPads()) {
        bool selectPad =
            pad->isSelectable() && pad->getGrabAreaScenePx().intersects(rectPx);
        pad->setSelected(selectFootprint || selectPad);
      }
      foreach (BI_StrokeText* text, footprint.getStrokeTexts()) {
        bool selectText = text->isSelectable() &&
                          text->getGrabAreaScenePx().intersects(rectPx);
        text->setSelected(selectFootprint || selectText);
      }
    }
    foreach (BI_NetSegment* segment, mNetSegments) {
      segment->setSelectionRect(rectPx);
    }
    foreach (BI_Plane* plane, mPlanes) {
      bool select = plane->isSelectable() &&
                    plane->getGrabAreaScenePx().intersects(rectPx);
      plane->setSelected(select);
    }
    foreach (BI_Polygon* polygon, mPolygons) {
      bool select = polygon->isSelectable() &&
                    polygon->getGrabAreaScenePx().intersects(rectPx);
      polygon->setSelected(select);
    }
    foreach (BI_StrokeText* text, mStrokeTexts) {
      bool select =
          text->isSelectable() && text->getGrabAreaScenePx().intersects(rectPx);
      text->setSelected(select);
    }
    foreach (BI_Hole* hole, mHoles) {
      bool select =
          hole->isSelectable() && hole->getGrabAreaScenePx().intersects(rectPx);
      hole->setSelected(select);
    }
  }
}

void Board::clearSelection() const noexcept {
  foreach (BI_Device* device, mDeviceInstances)
    device->getFootprint().setSelected(false);
  foreach (BI_NetSegment* segment, mNetSegments) { segment->clearSelection(); }
  foreach (BI_Plane* plane, mPlanes) { plane->setSelected(false); }
  foreach (BI_Polygon* polygon, mPolygons) { polygon->setSelected(false); }
  foreach (BI_StrokeText* text, mStrokeTexts) { text->setSelected(false); }
  foreach (BI_Hole* hole, mHoles) { hole->setSelected(false); }
}

std::unique_ptr<BoardSelectionQuery> Board::createSelectionQuery() const
    noexcept {
  return std::unique_ptr<BoardSelectionQuery>(new BoardSelectionQuery(
      mDeviceInstances, mNetSegments, mPlanes, mPolygons, mStrokeTexts, mHoles,
      const_cast<Board*>(this)));
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString Board::getBuiltInAttributeValue(const QString& key) const noexcept {
  if (key == QLatin1String("BOARD")) {
    return *mName;
  } else {
    return QString();
  }
}

QVector<const AttributeProvider*> Board::getAttributeProviderParents() const
    noexcept {
  return QVector<const AttributeProvider*>{&mProject};
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Board::updateIcon() noexcept {
  mIcon = QIcon(mGraphicsScene->toPixmap(QSize(297, 210), Qt::white));
}

void Board::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName, true);
  root.appendChild("default_font", mDefaultFontFileName, true);
  root.appendChild(mGridProperties->serializeToDomElement("grid"), true);
  root.appendChild(mLayerStack->serializeToDomElement("layers"), true);
  root.appendChild(mDesignRules->serializeToDomElement("design_rules"), true);
  root.appendChild(mFabricationOutputSettings->serializeToDomElement(
                       "fabrication_output_settings"),
                   true);
  root.appendLineBreak();
  serializePointerContainer(root, mDeviceInstances, "device");
  root.appendLineBreak();
  serializePointerContainerUuidSorted(root, mNetSegments, "netsegment");
  root.appendLineBreak();
  serializePointerContainerUuidSorted(root, mPlanes, "plane");
  root.appendLineBreak();
  serializePointerContainerUuidSorted(root, mPolygons, "polygon");
  root.appendLineBreak();
  serializePointerContainerUuidSorted(root, mStrokeTexts, "stroke_text");
  root.appendLineBreak();
  serializePointerContainerUuidSorted(root, mHoles, "hole");
  root.appendLineBreak();
}

void Board::updateErcMessages() noexcept {
  // type: UnplacedComponent (ComponentInstances without DeviceInstance)
  if (mIsAddedToProject) {
    const QMap<Uuid, ComponentInstance*>& componentInstances =
        mProject.getCircuit().getComponentInstances();
    foreach (const ComponentInstance* component, componentInstances) {
      if (component->getLibComponent().isSchematicOnly()) continue;
      BI_Device* device = mDeviceInstances.value(component->getUuid());
      ErcMsg*    ercMsg =
          mErcMsgListUnplacedComponentInstances.value(component->getUuid());
      if ((!device) && (!ercMsg)) {
        ErcMsg* ercMsg = new ErcMsg(
            mProject, *this,
            QString("%1/%2").arg(mUuid.toStr(), component->getUuid().toStr()),
            "UnplacedComponent", ErcMsg::ErcMsgType_t::BoardError,
            QString("Unplaced Component: %1 (Board: %2)")
                .arg(*component->getName(), *mName));
        ercMsg->setVisible(true);
        mErcMsgListUnplacedComponentInstances.insert(component->getUuid(),
                                                     ercMsg);
      } else if ((device) && (ercMsg)) {
        delete mErcMsgListUnplacedComponentInstances.take(component->getUuid());
      }
    }
    foreach (const Uuid& uuid, mErcMsgListUnplacedComponentInstances.keys()) {
      if (!componentInstances.contains(uuid))
        delete mErcMsgListUnplacedComponentInstances.take(uuid);
    }
  } else {
    qDeleteAll(mErcMsgListUnplacedComponentInstances);
    mErcMsgListUnplacedComponentInstances.clear();
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Board* Board::create(Project&                                project,
                     std::unique_ptr<TransactionalDirectory> directory,
                     const ElementName&                      name) {
  return new Board(project, std::move(directory), true, *name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
