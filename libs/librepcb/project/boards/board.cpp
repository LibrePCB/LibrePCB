/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "board.h"
#include <librepcb/common/application.h>
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/scopeguardlist.h>
#include <librepcb/common/boarddesignrules.h>
#include "../project.h"
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/gridproperties.h>
#include "../circuit/circuit.h"
#include "../erc/ercmsg.h"
#include "../circuit/componentinstance.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include "items/bi_via.h"
#include "items/bi_netsegment.h"
#include "items/bi_netpoint.h"
#include "items/bi_netline.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_hole.h"
#include "items/bi_plane.h"
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/pkg/footprint.h>
#include "items/bi_polygon.h"
#include "boardlayerstack.h"
#include "boardfabricationoutputsettings.h"
#include "boardusersettings.h"
#include "boardselectionquery.h"
#include "../circuit/netsignal.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Board::Board(const Board& other, const FilePath& filepath, const QString& name) :
    QObject(&other.getProject()), mProject(other.getProject()), mFilePath(filepath),
    mIsAddedToProject(false)
{
    try
    {
        mGraphicsScene.reset(new GraphicsScene());

        // copy the other board
        mFile.reset(SmartSExprFile::create(mFilePath));

        // set attributes
        mUuid = Uuid::createRandom();
        mName = name;
        mDefaultFontFileName = other.mDefaultFontFileName;

        // copy layer stack
        mLayerStack.reset(new BoardLayerStack(*this, *other.mLayerStack));

        // copy grid properties
        mGridProperties.reset(new GridProperties(*other.mGridProperties));

        // copy design rules
        mDesignRules.reset(new BoardDesignRules(*other.mDesignRules));

        // copy fabrication output settings
        mFabricationOutputSettings.reset(new BoardFabricationOutputSettings(
                                             *other.mFabricationOutputSettings));

        // copy user settings
        mUserSettings.reset(new BoardUserSettings(*this, *other.mUserSettings));

        // copy device instances
        QHash<const BI_Device*, BI_Device*> copiedDeviceInstances;
        foreach (const BI_Device* device, other.mDeviceInstances) {
            BI_Device* copy = new BI_Device(*this, *device);
            Q_ASSERT(!getDeviceInstanceByComponentUuid(copy->getComponentInstanceUuid()));
            mDeviceInstances.insert(copy->getComponentInstanceUuid(), copy);
            copiedDeviceInstances.insert(device, copy);
        }

        // copy netsegments
        foreach (const BI_NetSegment* netsegment, other.mNetSegments) {
            BI_NetSegment* copy = new BI_NetSegment(*this, *netsegment, copiedDeviceInstances);
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

        // rebuildAllPlanes(); --> fragments are copied too, so no need to rebuild them
        updateErcMessages();
        updateIcon();

        // emit the "attributesChanged" signal when the project has emited it
        connect(&mProject, &Project::attributesChanged, this, &Board::attributesChanged);

        connect(&mProject.getCircuit(), &Circuit::componentAdded, this, &Board::updateErcMessages);
        connect(&mProject.getCircuit(), &Circuit::componentRemoved, this, &Board::updateErcMessages);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        qDeleteAll(mErcMsgListUnplacedComponentInstances);    mErcMsgListUnplacedComponentInstances.clear();
        qDeleteAll(mHoles);             mHoles.clear();
        qDeleteAll(mStrokeTexts);       mStrokeTexts.clear();
        qDeleteAll(mPolygons);          mPolygons.clear();
        qDeleteAll(mPlanes);            mPlanes.clear();
        qDeleteAll(mNetSegments);       mNetSegments.clear();
        qDeleteAll(mDeviceInstances);   mDeviceInstances.clear();
        mUserSettings.reset();
        mFabricationOutputSettings.reset();
        mDesignRules.reset();
        mGridProperties.reset();
        mLayerStack.reset();
        mFile.reset();
        mGraphicsScene.reset();
        throw; // ...and rethrow the exception
    }
}

Board::Board(Project& project, const FilePath& filepath, bool restore,
             bool readOnly, bool create, const QString& newName) :
    QObject(&project), mProject(project), mFilePath(filepath), mIsAddedToProject(false)
{
    try
    {
        mGraphicsScene.reset(new GraphicsScene());

        // try to open/create the board file
        if (create)
        {
            mFile.reset(SmartSExprFile::create(mFilePath));

            // set attributes
            mUuid = Uuid::createRandom();
            mName = newName;
            mDefaultFontFileName = qApp->getDefaultStrokeFontName();

            // load default layer stack
            mLayerStack.reset(new BoardLayerStack(*this));

            // load default grid properties
            mGridProperties.reset(new GridProperties());

            // load default design rules
            mDesignRules.reset(new BoardDesignRules());

            // load default fabrication output settings
            mFabricationOutputSettings.reset(new BoardFabricationOutputSettings());

            // load default user settings
            mUserSettings.reset(new BoardUserSettings(*this, restore, readOnly, create));

            // add 160x100mm board outline (Eurocard size)
            Polygon polygon(Uuid::createRandom(), GraphicsLayer::sBoardOutlines, Length(0),
                false, false, Path::rect(Point(0, 0), Point(160000000, 100000000)));
            mPolygons.append(new BI_Polygon(*this, polygon));
        }
        else
        {
            mFile.reset(new SmartSExprFile(mFilePath, restore, readOnly));
            SExpression root = mFile->parseFileAndBuildDomTree();

            // the board seems to be ready to open, so we will create all needed objects

            if (root.getChildByIndex(0).isString()) {
                mUuid = root.getChildByIndex(0).getValue<Uuid>(true);
            } else {
                // backward compatibility, remove this some time!
                mUuid = root.getValueByPath<Uuid>("uuid", true);
            }
            mName = root.getValueByPath<QString>("name", true);
            if (const SExpression* child = root.tryGetChildByPath("default_font")) {
                mDefaultFontFileName = child->getValueOfFirstChild<QString>(true);
            } else {
                mDefaultFontFileName = qApp->getDefaultStrokeFontName();
            }

            // Load grid properties
            mGridProperties.reset(new GridProperties(root.getChildByPath("grid")));

            // Load layer stack
            mLayerStack.reset(new BoardLayerStack(*this, root.getChildByPath("layers")));

            // load design rules
            mDesignRules.reset(new BoardDesignRules(root.getChildByPath("design_rules")));

            // load fabrication output settings
            if (const SExpression* child = root.tryGetChildByPath("fabrication_output_settings")) {
                mFabricationOutputSettings.reset(new BoardFabricationOutputSettings(*child));
            } else {
                // backward compatibility - remove this some time!
                mFabricationOutputSettings.reset(new BoardFabricationOutputSettings());
            }

            // load user settings
            mUserSettings.reset(new BoardUserSettings(*this, restore, readOnly, create));

            // Load all device instances
            foreach (const SExpression& node, root.getChildren("device")) {
                BI_Device* device = new BI_Device(*this, node);
                if (getDeviceInstanceByComponentUuid(device->getComponentInstanceUuid())) {
                    throw RuntimeError(__FILE__, __LINE__,
                        QString(tr("There is already a device of the component instance \"%1\"!"))
                        .arg(device->getComponentInstanceUuid().toStr()));
                }
                mDeviceInstances.insert(device->getComponentInstanceUuid(), device);
            }

            // Load all netsegments
            foreach (const SExpression& node, root.getChildren("netsegment")) {
                BI_NetSegment* netsegment = new BI_NetSegment(*this, node);
                if (getNetSegmentByUuid(netsegment->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__,
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

            //////////////////////////////////////////////////////////////////////////////
            // TODO: Backward compatibility, remove this some time!
            int strokeTextCount = mStrokeTexts.count();
            foreach (const BI_Device* device, mDeviceInstances) {
                foreach (const BI_StrokeText* text, device->getFootprint().getStrokeTexts()) {
                    Q_UNUSED(text);
                    ++strokeTextCount;
                }
            }
            if (strokeTextCount == 0) {
                foreach (const BI_Device* device, mDeviceInstances) {
                    device->getFootprint().resetStrokeTextsToLibraryFootprint(); // can throw
                }
            }
            //////////////////////////////////////////////////////////////////////////////
        }

        rebuildAllPlanes();
        updateErcMessages();
        updateIcon();

        // emit the "attributesChanged" signal when the project has emited it
        connect(&mProject, &Project::attributesChanged, this, &Board::attributesChanged);

        connect(&mProject.getCircuit(), &Circuit::componentAdded, this, &Board::updateErcMessages);
        connect(&mProject.getCircuit(), &Circuit::componentRemoved, this, &Board::updateErcMessages);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        qDeleteAll(mErcMsgListUnplacedComponentInstances);    mErcMsgListUnplacedComponentInstances.clear();
        qDeleteAll(mHoles);             mHoles.clear();
        qDeleteAll(mStrokeTexts);       mStrokeTexts.clear();
        qDeleteAll(mPolygons);          mPolygons.clear();
        qDeleteAll(mPlanes);            mPlanes.clear();
        qDeleteAll(mNetSegments);       mNetSegments.clear();
        qDeleteAll(mDeviceInstances);   mDeviceInstances.clear();
        mUserSettings.reset();
        mFabricationOutputSettings.reset();
        mDesignRules.reset();
        mGridProperties.reset();
        mLayerStack.reset();
        mFile.reset();
        mGraphicsScene.reset();
        throw; // ...and rethrow the exception
    }
}

Board::~Board() noexcept
{
    Q_ASSERT(!mIsAddedToProject);

    qDeleteAll(mErcMsgListUnplacedComponentInstances);    mErcMsgListUnplacedComponentInstances.clear();

    // delete all items
    qDeleteAll(mHoles);             mHoles.clear();
    qDeleteAll(mStrokeTexts);       mStrokeTexts.clear();
    qDeleteAll(mPolygons);          mPolygons.clear();
    qDeleteAll(mPlanes);            mPlanes.clear();
    qDeleteAll(mNetSegments);       mNetSegments.clear();
    qDeleteAll(mDeviceInstances);   mDeviceInstances.clear();

    mUserSettings.reset();
    mFabricationOutputSettings.reset();
    mDesignRules.reset();
    mGridProperties.reset();
    mLayerStack.reset();
    mFile.reset();
    mGraphicsScene.reset();
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

bool Board::isEmpty() const noexcept
{
    return (mDeviceInstances.isEmpty() &&
            mNetSegments.isEmpty() &&
            mPlanes.isEmpty() &&
            mPolygons.isEmpty() &&
            mStrokeTexts.isEmpty() &&
            mHoles.isEmpty());
}

QList<BI_Base*> Board::getItemsAtScenePos(const Point& pos) const noexcept
{
    QPointF scenePosPx = pos.toPxQPointF();
    QList<BI_Base*> list;   // Note: The order of adding the items is very important (the
                            // top most item must appear as the first item in the list)!
    // vias
    foreach (BI_Via* via, getViasAtScenePos(pos, nullptr)) {
        list.append(via);
    }
    // netpoints
    foreach (BI_NetPoint* netpoint, getNetPointsAtScenePos(pos, nullptr, nullptr)) {
        list.append(netpoint);
    }
    // netlines
    foreach (BI_NetLine* netline, getNetLinesAtScenePos(pos, nullptr, nullptr)) {
        list.append(netline);
    }
    // footprints & pads
    foreach (BI_Device* device, mDeviceInstances) {
        BI_Footprint& footprint = device->getFootprint();
        if (footprint.isSelectable() && footprint.getGrabAreaScenePx().contains(scenePosPx)) {
            if (footprint.getIsMirrored()) {
                list.append(&footprint);
            } else {
                list.prepend(&footprint);
            }
        }
        foreach (BI_FootprintPad* pad, footprint.getPads()) {
            if (pad->isSelectable() && pad->getGrabAreaScenePx().contains(scenePosPx)) {
                if (pad->getIsMirrored()) {
                    list.append(pad);
                } else {
                    list.insert(1, pad);
                }
            }
        }
        foreach (BI_StrokeText* text, device->getFootprint().getStrokeTexts()) {
            if (text->isSelectable() && text->getGrabAreaScenePx().contains(scenePosPx)) {
                if (GraphicsLayer::isTopLayer(text->getText().getLayerName())) {
                    list.prepend(text);
                } else {
                    list.append(text);
                }
            }
        }
    }
    // planes
    foreach (BI_Plane* planes, mPlanes) {
        if (planes->isSelectable() && planes->getGrabAreaScenePx().contains(scenePosPx)) {
            list.append(planes);
        }
    }
    // polygons
    foreach (BI_Polygon* polygon, mPolygons) {
        if (polygon->isSelectable() && polygon->getGrabAreaScenePx().contains(scenePosPx)) {
            list.append(polygon);
        }
    }
    // texts
    foreach (BI_StrokeText* text, mStrokeTexts) {
        if (text->isSelectable() && text->getGrabAreaScenePx().contains(scenePosPx)) {
            list.append(text);
        }
    }
    // holes
    foreach (BI_Hole* hole, mHoles) {
        if (hole->isSelectable() && hole->getGrabAreaScenePx().contains(scenePosPx)) {
            list.append(hole);
        }
    }
    return list;
}

QList<BI_Via*> Board::getViasAtScenePos(const Point& pos, const NetSignal* netsignal) const noexcept
{
    QList<BI_Via*> list;
    foreach (BI_NetSegment* segment, mNetSegments) {
        if ((!netsignal) || (&segment->getNetSignal() == netsignal)) {
            segment->getViasAtScenePos(pos, list);
        }
    }
    return list;
}

QList<BI_NetPoint*> Board::getNetPointsAtScenePos(const Point& pos, const GraphicsLayer* layer,
                                                  const NetSignal* netsignal) const noexcept
{
    QList<BI_NetPoint*> list;
    foreach (BI_NetSegment* segment, mNetSegments) {
        if ((!netsignal) || (&segment->getNetSignal() == netsignal)) {
            segment->getNetPointsAtScenePos(pos, layer, list);
        }
    }
    return list;
}

QList<BI_NetLine*> Board::getNetLinesAtScenePos(const Point& pos, const GraphicsLayer* layer,
                                                const NetSignal* netsignal) const noexcept
{
    QList<BI_NetLine*> list;
    foreach (BI_NetSegment* segment, mNetSegments) {
        if ((!netsignal) || (&segment->getNetSignal() == netsignal)) {
            segment->getNetLinesAtScenePos(pos, layer, list);
        }
    }
    return list;
}

QList<BI_FootprintPad*> Board::getPadsAtScenePos(const Point& pos, const GraphicsLayer* layer,
                                                 const NetSignal* netsignal) const noexcept
{
    QList<BI_FootprintPad*> list;
    foreach (BI_Device* device, mDeviceInstances)
    {
        foreach (BI_FootprintPad* pad, device->getFootprint().getPads())
        {
            if (pad->isSelectable() && pad->getGrabAreaScenePx().contains(pos.toPxQPointF())
                && ((!layer) || (pad->isOnLayer(layer->getName())))
                && ((!netsignal) || (pad->getCompSigInstNetSignal() == netsignal)))
            {
                list.append(pad);
            }
        }
    }
    return list;
}

QList<BI_Base*> Board::getAllItems() const noexcept
{
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
    return items;
}

/*****************************************************************************************
 *  Setters: General
 ****************************************************************************************/

void Board::setGridProperties(const GridProperties& grid) noexcept
{
    *mGridProperties = grid;
}

/*****************************************************************************************
 *  DeviceInstance Methods
 ****************************************************************************************/

BI_Device* Board::getDeviceInstanceByComponentUuid(const Uuid& uuid) const noexcept
{
    return mDeviceInstances.value(uuid, nullptr);
}

void Board::addDeviceInstance(BI_Device& instance)
{
    if ((!mIsAddedToProject) || (&instance.getBoard() != this)) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no device with the same component instance in the list
    if (getDeviceInstanceByComponentUuid(instance.getComponentInstance().getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a device with the component instance \"%1\"!"))
            .arg(instance.getComponentInstance().getUuid().toStr()));
    }
    // add to board
    instance.addToBoard(); // can throw
    mDeviceInstances.insert(instance.getComponentInstanceUuid(), &instance);
    updateErcMessages();
    emit deviceAdded(instance);
}

void Board::removeDeviceInstance(BI_Device& instance)
{
    if ((!mIsAddedToProject) || (!mDeviceInstances.contains(instance.getComponentInstanceUuid()))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    instance.removeFromBoard(); // can throw
    mDeviceInstances.remove(instance.getComponentInstanceUuid());
    updateErcMessages();
    emit deviceRemoved(instance);
}

/*****************************************************************************************
 *  NetSegment Methods
 ****************************************************************************************/

BI_NetSegment* Board::getNetSegmentByUuid(const Uuid& uuid) const noexcept
{
    foreach (BI_NetSegment* netsegment, mNetSegments) {
        if (netsegment->getUuid() == uuid)
            return netsegment;
    }
    return nullptr;
}

void Board::addNetSegment(BI_NetSegment& netsegment)
{
    if ((!mIsAddedToProject) || (mNetSegments.contains(&netsegment))
        || (&netsegment.getBoard() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netsegment with the same uuid in the list
    if (getNetSegmentByUuid(netsegment.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a netsegment with the UUID \"%1\"!"))
            .arg(netsegment.getUuid().toStr()));
    }
    // add to board
    netsegment.addToBoard(); // can throw
    mNetSegments.append(&netsegment);
}

void Board::removeNetSegment(BI_NetSegment& netsegment)
{
    if ((!mIsAddedToProject) || (!mNetSegments.contains(&netsegment))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    netsegment.removeFromBoard(); // can throw
    mNetSegments.removeOne(&netsegment);
}

/*****************************************************************************************
 *  Plane Methods
 ****************************************************************************************/

void Board::addPlane(BI_Plane& plane)
{
    if ((!mIsAddedToProject) || (mPlanes.contains(&plane))
        || (&plane.getBoard() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    plane.addToBoard(); // can throw
    mPlanes.append(&plane);
}

void Board::removePlane(BI_Plane& plane)
{
    if ((!mIsAddedToProject) || (!mPlanes.contains(&plane))) {
        throw LogicError(__FILE__, __LINE__);
    }
    plane.removeFromBoard(); // can throw
    mPlanes.removeOne(&plane);
}

void Board::rebuildAllPlanes() noexcept
{
    QList<BI_Plane*> planes = mPlanes;
    qSort(planes.begin(), planes.end(),
          [](const BI_Plane* p1, const BI_Plane* p2)
          {return !(*p1 < *p2);}); // sort by priority (highest priority first)
    foreach (BI_Plane* plane, planes) {
        plane->rebuild();
    }
}

/*****************************************************************************************
 *  Polygon Methods
 ****************************************************************************************/

void Board::addPolygon(BI_Polygon& polygon)
{
    if ((!mIsAddedToProject) || (mPolygons.contains(&polygon))
        || (&polygon.getBoard() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    polygon.addToBoard(); // can throw
    mPolygons.append(&polygon);
}

void Board::removePolygon(BI_Polygon& polygon)
{
    if ((!mIsAddedToProject) || (!mPolygons.contains(&polygon))) {
        throw LogicError(__FILE__, __LINE__);
    }
    polygon.removeFromBoard(); // can throw
    mPolygons.removeOne(&polygon);
}

/*****************************************************************************************
 *  StrokeText Methods
 ****************************************************************************************/

void Board::addStrokeText(BI_StrokeText& text)
{
    if ((!mIsAddedToProject) || (mStrokeTexts.contains(&text))
        || (&text.getBoard() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    text.addToBoard(); // can throw
    mStrokeTexts.append(&text);
}

void Board::removeStrokeText(BI_StrokeText& text)
{
    if ((!mIsAddedToProject) || (!mStrokeTexts.contains(&text))) {
        throw LogicError(__FILE__, __LINE__);
    }
    text.removeFromBoard(); // can throw
    mStrokeTexts.removeOne(&text);
}

/*****************************************************************************************
 *  Hole Methods
 ****************************************************************************************/

void Board::addHole(BI_Hole& hole)
{
    if ((!mIsAddedToProject) || (mHoles.contains(&hole))
        || (&hole.getBoard() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    hole.addToBoard(); // can throw
    mHoles.append(&hole);
}

void Board::removeHole(BI_Hole& hole)
{
    if ((!mIsAddedToProject) || (!mHoles.contains(&hole))) {
        throw LogicError(__FILE__, __LINE__);
    }
    hole.removeFromBoard(); // can throw
    mHoles.removeOne(&hole);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Board::addToProject()
{
    if (mIsAddedToProject) {
        throw LogicError(__FILE__, __LINE__);
    }
    QList<BI_Base*> items = getAllItems();
    ScopeGuardList sgl(items.count());
    for (int i = 0; i < items.count(); ++i) {
        BI_Base* item = items.at(i);
        item->addToBoard(); // can throw
        sgl.add([item](){item->removeFromBoard();});
    }
    mIsAddedToProject = true;
    updateErcMessages();
    sgl.dismiss();
}

void Board::removeFromProject()
{
    if (!mIsAddedToProject) {
        throw LogicError(__FILE__, __LINE__);
    }
    QList<BI_Base*> items = getAllItems();
    ScopeGuardList sgl(items.count());
    for (int i = items.count()-1; i >= 0; --i) {
        BI_Base* item = items.at(i);
        item->removeFromBoard(); // can throw
        sgl.add([item](){item->addToBoard();});
    }
    mIsAddedToProject = false;
    updateErcMessages();
    sgl.dismiss();
}

bool Board::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // save board file
    try
    {
        if (mIsAddedToProject)
        {
            SExpression doc(serializeToDomElement("librepcb_board"));
            mFile->save(doc, toOriginal);
        }
        else
        {
            mFile->removeFile(toOriginal);
        }
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getMsg());
    }

    // save user settings
    if (!mUserSettings->save(toOriginal, errors)) {
        success = false;
    }

    return success;
}

void Board::showInView(GraphicsView& view) noexcept
{
    view.setScene(mGraphicsScene.data());
}

void Board::setSelectionRect(const Point& p1, const Point& p2, bool updateItems) noexcept
{
    mGraphicsScene->setSelectionRect(p1, p2);
    if (updateItems) {
        QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
        foreach (BI_Device* component, mDeviceInstances) {
            BI_Footprint& footprint = component->getFootprint();
            bool selectFootprint = footprint.isSelectable() && footprint.getGrabAreaScenePx().intersects(rectPx);
            footprint.setSelected(selectFootprint);
            foreach (BI_FootprintPad* pad, footprint.getPads()) {
                bool selectPad = pad->isSelectable() && pad->getGrabAreaScenePx().intersects(rectPx);
                pad->setSelected(selectFootprint || selectPad);
            }
            foreach (BI_StrokeText* text, footprint.getStrokeTexts()) {
                bool selectText = text->isSelectable() && text->getGrabAreaScenePx().intersects(rectPx);
                text->setSelected(selectFootprint || selectText);
            }
        }
        foreach (BI_NetSegment* segment, mNetSegments) {
            segment->setSelectionRect(rectPx);
        }
        foreach (BI_Plane* plane, mPlanes) {
            bool select = plane->isSelectable() && plane->getGrabAreaScenePx().intersects(rectPx);
            plane->setSelected(select);
        }
        foreach (BI_Polygon* polygon, mPolygons) {
            bool select = polygon->isSelectable() && polygon->getGrabAreaScenePx().intersects(rectPx);
            polygon->setSelected(select);
        }
        foreach (BI_StrokeText* text, mStrokeTexts) {
            bool select = text->isSelectable() && text->getGrabAreaScenePx().intersects(rectPx);
            text->setSelected(select);
        }
        foreach (BI_Hole* hole, mHoles) {
            bool select = hole->isSelectable() && hole->getGrabAreaScenePx().intersects(rectPx);
            hole->setSelected(select);
        }
    }
}

void Board::clearSelection() const noexcept
{
    foreach (BI_Device* device, mDeviceInstances)
        device->getFootprint().setSelected(false);
    foreach (BI_NetSegment* segment, mNetSegments) {
        segment->clearSelection();
    }
    foreach (BI_Plane* plane, mPlanes) {
        plane->setSelected(false);
    }
    foreach (BI_Polygon* polygon, mPolygons) {
        polygon->setSelected(false);
    }
    foreach (BI_StrokeText* text, mStrokeTexts) {
        text->setSelected(false);
    }
    foreach (BI_Hole* hole, mHoles) {
        hole->setSelected(false);
    }
}

std::unique_ptr<BoardSelectionQuery> Board::createSelectionQuery() const noexcept
{
    return std::unique_ptr<BoardSelectionQuery>(
        new BoardSelectionQuery(mDeviceInstances, mNetSegments, mPlanes, mPolygons,
                                mStrokeTexts, mHoles, const_cast<Board*>(this)));
}

/*****************************************************************************************
 *  Inherited from AttributeProvider
 ****************************************************************************************/

QString Board::getBuiltInAttributeValue(const QString& key) const noexcept
{
    if (key == QLatin1String("BOARD")) {
        return mName;
    } else {
        return QString();
    }
}

QVector<const AttributeProvider*> Board::getAttributeProviderParents() const noexcept
{
    return QVector<const AttributeProvider*>{&mProject};
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Board::updateIcon() noexcept
{
    QRectF source = mGraphicsScene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
    QRect target(0, 0, 297, 210); // DIN A4 format :-)

    QPixmap pixmap(target.size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    mGraphicsScene->render(&painter, target, source);
    mIcon = QIcon(pixmap);
}

bool Board::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mName.isEmpty())    return false;
    return true;
}

void Board::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendStringChild("name", mName, true);
    root.appendStringChild("default_font", mDefaultFontFileName, true);
    root.appendChild(mGridProperties->serializeToDomElement("grid"), true);
    root.appendChild(mLayerStack->serializeToDomElement("layers"), true);
    root.appendChild(mDesignRules->serializeToDomElement("design_rules"), true);
    root.appendChild(mFabricationOutputSettings->serializeToDomElement("fabrication_output_settings"), true);
    root.appendLineBreak();
    serializePointerContainer(root, mDeviceInstances, "device");
    root.appendLineBreak();
    serializePointerContainer(root, mNetSegments, "netsegment");
    root.appendLineBreak();
    serializePointerContainer(root, mPlanes, "plane");
    root.appendLineBreak();
    serializePointerContainer(root, mPolygons, "polygon");
    root.appendLineBreak();
    serializePointerContainer(root, mStrokeTexts, "stroke_text");
    root.appendLineBreak();
    serializePointerContainer(root, mHoles, "hole");
    root.appendLineBreak();
}

void Board::updateErcMessages() noexcept
{
    // type: UnplacedComponent (ComponentInstances without DeviceInstance)
    if (mIsAddedToProject)
    {
        const QMap<Uuid, ComponentInstance*>& componentInstances = mProject.getCircuit().getComponentInstances();
        foreach (const ComponentInstance* component, componentInstances)
        {
            if (component->getLibComponent().isSchematicOnly()) continue;
            BI_Device* device = mDeviceInstances.value(component->getUuid());
            ErcMsg* ercMsg = mErcMsgListUnplacedComponentInstances.value(component->getUuid());
            if ((!device) && (!ercMsg))
            {
                ErcMsg* ercMsg = new ErcMsg(mProject, *this, QString("%1/%2").arg(mUuid.toStr(),
                    component->getUuid().toStr()), "UnplacedComponent", ErcMsg::ErcMsgType_t::BoardError,
                    QString("Unplaced Component: %1 (Board: %2)").arg(component->getName(), mName));
                ercMsg->setVisible(true);
                mErcMsgListUnplacedComponentInstances.insert(component->getUuid(), ercMsg);
            }
            else if ((device) && (ercMsg))
            {
                delete mErcMsgListUnplacedComponentInstances.take(component->getUuid());
            }
        }
        foreach (const Uuid& uuid, mErcMsgListUnplacedComponentInstances.keys()) {
            if (!componentInstances.contains(uuid))
                delete mErcMsgListUnplacedComponentInstances.take(uuid);
        }
    }
    else
    {
        qDeleteAll(mErcMsgListUnplacedComponentInstances);
        mErcMsgListUnplacedComponentInstances.clear();
    }
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Board* Board::create(Project& project, const FilePath& filepath, const QString& name)
{
    return new Board(project, filepath, false, false, true, name);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
