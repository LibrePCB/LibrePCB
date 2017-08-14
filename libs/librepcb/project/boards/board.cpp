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
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/domdocument.h>
#include <librepcb/common/scopeguardlist.h>
#include <librepcb/common/boarddesignrules.h>
#include <librepcb/common/boardlayer.h>
#include "../project.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/gridproperties.h>
#include "../circuit/circuit.h"
#include "../erc/ercmsg.h"
#include "../circuit/componentinstance.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include "items/bi_via.h"
#include "items/bi_netpoint.h"
#include "items/bi_netline.h"
#include <librepcb/library/cmp/component.h>
#include "items/bi_polygon.h"
#include "boardlayerstack.h"

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
        mXmlFile.reset(SmartXmlFile::create(mFilePath));

        // set attributes
        mUuid = Uuid::createRandom();
        mName = name;

        // copy layer stack
        mLayerStack.reset(new BoardLayerStack(*this, *other.mLayerStack));

        // copy grid properties
        mGridProperties.reset(new GridProperties(*other.mGridProperties));

        // copy design rules
        mDesignRules.reset(new BoardDesignRules(*other.mDesignRules));

        // copy device instances
        QHash<const BI_Device*, BI_Device*> copiedDeviceInstances;
        foreach (const BI_Device* device, other.mDeviceInstances) {
            BI_Device* copy = new BI_Device(*this, *device);
            Q_ASSERT(!getDeviceInstanceByComponentUuid(copy->getComponentInstanceUuid()));
            mDeviceInstances.insert(copy->getComponentInstanceUuid(), copy);
            copiedDeviceInstances.insert(device, copy);
        }

        // copy vias
        QHash<const BI_Via*, BI_Via*> copiedVias;
        foreach (const BI_Via* via, other.mVias) {
            BI_Via* copy = new BI_Via(*this, *via);
            Q_ASSERT(!getViaByUuid(copy->getUuid()));
            mVias.append(copy);
            copiedVias.insert(via, copy);
        }

        // copy netpoints
        QHash<const BI_NetPoint*, BI_NetPoint*> copiedNetPoints;
        foreach (const BI_NetPoint* netpoint, other.mNetPoints) {
            BI_FootprintPad* pad = nullptr;
            if (netpoint->getFootprintPad()) {
                const BI_Device* oldDevice = &netpoint->getFootprintPad()->getFootprint().getDeviceInstance();
                const BI_Device* newDevice = copiedDeviceInstances.value(oldDevice, nullptr);
                Q_ASSERT(newDevice);
                pad = newDevice->getFootprint().getPad(netpoint->getFootprintPad()->getLibPadUuid());
                Q_ASSERT(pad);
            }
            BI_Via* via = copiedVias.value(netpoint->getVia(), nullptr);
            BI_NetPoint* copy = new BI_NetPoint(*this, *netpoint, pad, via);
            Q_ASSERT(!getNetPointByUuid(copy->getUuid()));
            mNetPoints.append(copy);
            copiedNetPoints.insert(netpoint, copy);
        }

        // copy netlines
        foreach (const BI_NetLine* netline, other.mNetLines) {
            BI_NetPoint* start = copiedNetPoints.value(&netline->getStartPoint());
            BI_NetPoint* end = copiedNetPoints.value(&netline->getEndPoint());
            Q_ASSERT(start && end);
            BI_NetLine* copy = new BI_NetLine(*this, *netline, *start, *end);
            Q_ASSERT(!getNetLineByUuid(copy->getUuid()));
            mNetLines.append(copy);
        }

        // copy polygons
        foreach (const BI_Polygon* polygon, other.mPolygons) {
            BI_Polygon* copy = new BI_Polygon(*this, *polygon);
            mPolygons.append(copy);
        }

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
        qDeleteAll(mPolygons);          mPolygons.clear();
        qDeleteAll(mNetLines);          mNetLines.clear();
        qDeleteAll(mNetPoints);         mNetPoints.clear();
        qDeleteAll(mVias);              mVias.clear();
        qDeleteAll(mDeviceInstances);   mDeviceInstances.clear();
        mDesignRules.reset();
        mGridProperties.reset();
        mLayerStack.reset();
        mXmlFile.reset();
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

        // try to open/create the XML board file
        if (create)
        {
            mXmlFile.reset(SmartXmlFile::create(mFilePath));

            // set attributes
            mUuid = Uuid::createRandom();
            mName = newName;

            // load default layer stack
            mLayerStack.reset(new BoardLayerStack(*this));

            // load default grid properties
            mGridProperties.reset(new GridProperties());

            // load default design rules
            mDesignRules.reset(new BoardDesignRules());
        }
        else
        {
            mXmlFile.reset(new SmartXmlFile(mFilePath, restore, readOnly));
            std::unique_ptr<DomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
            DomElement& root = doc->getRoot();

            // the board seems to be ready to open, so we will create all needed objects

            mUuid = root.getFirstChild("uuid", true)->getText<Uuid>(true);
            mName = root.getFirstChild("name", true)->getText<QString>(true);

            // Load grid properties
            mGridProperties.reset(new GridProperties(*root.getFirstChild("grid", true)));

            // Load layer stack
            mLayerStack.reset(new BoardLayerStack(*this, *root.getFirstChild("layers", true)));

            // load design rules
            mDesignRules.reset(new BoardDesignRules(*root.getFirstChild("design_rules", true)));

            // Load all device instances
            foreach (const DomElement* node, root.getChilds("device")) {
                BI_Device* device = new BI_Device(*this, *node);
                if (getDeviceInstanceByComponentUuid(device->getComponentInstanceUuid())) {
                    throw RuntimeError(__FILE__, __LINE__,
                        QString(tr("There is already a device of the component instance \"%1\"!"))
                        .arg(device->getComponentInstanceUuid().toStr()));
                }
                mDeviceInstances.insert(device->getComponentInstanceUuid(), device);
            }

            // Load all vias
            foreach (const DomElement* node, root.getChilds("via")) {
                BI_Via* via = new BI_Via(*this, *node);
                if (getViaByUuid(via->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__,
                        QString(tr("There is already a via with the UUID \"%1\"!"))
                        .arg(via->getUuid().toStr()));
                }
                mVias.append(via);
            }

            // Load all netpoints
            foreach (const DomElement* node, root.getChilds("netpoint")) {
                BI_NetPoint* netpoint = new BI_NetPoint(*this, *node);
                if (getNetPointByUuid(netpoint->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__,
                        QString(tr("There is already a netpoint with the UUID \"%1\"!"))
                        .arg(netpoint->getUuid().toStr()));
                }
                mNetPoints.append(netpoint);
            }

            // Load all netlines
            foreach (const DomElement* node, root.getChilds("netline")) {
                BI_NetLine* netline = new BI_NetLine(*this, *node);
                if (getNetLineByUuid(netline->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__,
                        QString(tr("There is already a netline with the UUID \"%1\"!"))
                        .arg(netline->getUuid().toStr()));
                }
                mNetLines.append(netline);
            }

            // Load all polygons
            foreach (const DomElement* node, root.getChilds("polygon")) {
                BI_Polygon* polygon = new BI_Polygon(*this, *node);
                mPolygons.append(polygon);
            }
        }

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
        qDeleteAll(mPolygons);          mPolygons.clear();
        qDeleteAll(mNetLines);          mNetLines.clear();
        qDeleteAll(mNetPoints);         mNetPoints.clear();
        qDeleteAll(mVias);              mVias.clear();
        qDeleteAll(mDeviceInstances);   mDeviceInstances.clear();
        mDesignRules.reset();
        mGridProperties.reset();
        mLayerStack.reset();
        mXmlFile.reset();
        mGraphicsScene.reset();
        throw; // ...and rethrow the exception
    }
}

Board::~Board() noexcept
{
    Q_ASSERT(!mIsAddedToProject);

    qDeleteAll(mErcMsgListUnplacedComponentInstances);    mErcMsgListUnplacedComponentInstances.clear();

    // delete all items
    qDeleteAll(mPolygons);          mPolygons.clear();
    qDeleteAll(mNetLines);          mNetLines.clear();
    qDeleteAll(mNetPoints);         mNetPoints.clear();
    qDeleteAll(mVias);              mVias.clear();
    qDeleteAll(mDeviceInstances);   mDeviceInstances.clear();

    mDesignRules.reset();
    mGridProperties.reset();
    mLayerStack.reset();
    mXmlFile.reset();
    mGraphicsScene.reset();
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

bool Board::isEmpty() const noexcept
{
    return (mDeviceInstances.isEmpty() &&
            mVias.isEmpty() &&
            mNetPoints.isEmpty() &&
            mNetLines.isEmpty());
}

QList<BI_Base*> Board::getSelectedItems(bool vias,
                                        bool footprintPads,
                                        bool floatingPoints,
                                        bool attachedPoints,
                                        bool floatingPointsFromFloatingLines,
                                        bool attachedPointsFromFloatingLines,
                                        bool floatingPointsFromAttachedLines,
                                        bool attachedPointsFromAttachedLines,
                                        bool attachedPointsFromFootprints,
                                        bool floatingLines,
                                        bool attachedLines,
                                        bool attachedLinesFromFootprints) const noexcept
{
    // TODO: this method is incredible ugly ;)

    QList<BI_Base*> list;
    foreach (BI_Device* component, mDeviceInstances)
    {
        BI_Footprint& footprint = component->getFootprint();

        // footprint
        if (footprint.isSelected())
            list.append(&footprint);

        // pads
        foreach (BI_FootprintPad* pad, footprint.getPads())
        {
            // pad
            if (pad->isSelected() && footprintPads)
                list.append(pad);

            // attached netpoints & netlines
            foreach (BI_NetPoint* attachedNetPoint, pad->getNetPoints()) {
                if (footprint.isSelected() && attachedPointsFromFootprints && attachedNetPoint)
                {
                    if (!list.contains(attachedNetPoint))
                        list.append(attachedNetPoint);
                }
                if (footprint.isSelected() && attachedLinesFromFootprints && attachedNetPoint)
                {
                    foreach (BI_NetLine* attachedNetLine, attachedNetPoint->getLines())
                    {
                        if (!list.contains(attachedNetLine))
                            list.append(attachedNetLine);
                    }
                }
            }
        }
    }
    foreach (BI_Via* via, mVias)
    {
        if (via->isSelected() && vias) {
            list.append(via);
        }
    }
    foreach (BI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->isSelected())
        {
            if (((!netpoint->isAttached()) && floatingPoints)
               || (netpoint->isAttached() && attachedPoints))
            {
                if (!list.contains(netpoint))
                    list.append(netpoint);
            }
        }
    }
    foreach (BI_NetLine* netline, mNetLines)
    {
        if (netline->isSelected())
        {
            // netline
            if (((!netline->isAttached()) && floatingLines)
               || (netline->isAttached() && attachedLines))
            {
                if (!list.contains(netline))
                    list.append(netline);
            }
            // netpoints from netlines
            BI_NetPoint* p1 = &netline->getStartPoint();
            BI_NetPoint* p2 = &netline->getEndPoint();
            if ( ((!netline->isAttached()) && (!p1->isAttached()) && floatingPointsFromFloatingLines)
              || ((!netline->isAttached()) && ( p1->isAttached()) && attachedPointsFromFloatingLines)
              || (( netline->isAttached()) && (!p1->isAttached()) && floatingPointsFromAttachedLines)
              || (( netline->isAttached()) && ( p1->isAttached()) && attachedPointsFromAttachedLines))
            {
                if (!list.contains(p1))
                    list.append(p1);
            }
            if ( ((!netline->isAttached()) && (!p2->isAttached()) && floatingPointsFromFloatingLines)
              || ((!netline->isAttached()) && ( p2->isAttached()) && attachedPointsFromFloatingLines)
              || (( netline->isAttached()) && (!p2->isAttached()) && floatingPointsFromAttachedLines)
              || (( netline->isAttached()) && ( p2->isAttached()) && attachedPointsFromAttachedLines))
            {
                if (!list.contains(p2))
                    list.append(p2);
            }
        }
    }

    return list;
}

QList<BI_Base*> Board::getItemsAtScenePos(const Point& pos) const noexcept
{
    QPointF scenePosPx = pos.toPxQPointF();
    QList<BI_Base*> list;   // Note: The order of adding the items is very important (the
                            // top most item must appear as the first item in the list)!
    // vias
    foreach (BI_Via* via, mVias)
    {
        if (via->isSelectable() && via->getGrabAreaScenePx().contains(scenePosPx)) {
            list.append(via);
        }
    }
    // netpoints
    foreach (BI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->isSelectable() && netpoint->getGrabAreaScenePx().contains(scenePosPx)) {
            list.append(netpoint);
        }
    }
    // netlines
    foreach (BI_NetLine* netline, mNetLines)
    {
        if (netline->isSelectable() && netline->getGrabAreaScenePx().contains(scenePosPx)) {
            list.append(netline);
        }
    }
    // footprints & pads
    foreach (BI_Device* device, mDeviceInstances)
    {
        BI_Footprint& footprint = device->getFootprint();
        if (footprint.isSelectable() && footprint.getGrabAreaScenePx().contains(scenePosPx)) {
            if (footprint.getIsMirrored()) {
                list.append(&footprint);
            } else {
                list.prepend(&footprint);
            }
        }
        foreach (BI_FootprintPad* pad, footprint.getPads())
        {
            if (pad->isSelectable() && pad->getGrabAreaScenePx().contains(scenePosPx)) {
                if (pad->getIsMirrored()) {
                    list.append(pad);
                } else {
                    list.insert(1, pad);
                }
            }
        }
    }
    return list;
}

QList<BI_Via*> Board::getViasAtScenePos(const Point& pos, const NetSignal* netsignal) const noexcept
{
    QList<BI_Via*> list;
    foreach (BI_Via* via, mVias)
    {
        if (via->isSelectable() && via->getGrabAreaScenePx().contains(pos.toPxQPointF())
            && ((!netsignal) || (via->getNetSignal() == netsignal)))
        {
            list.append(via);
        }
    }
    return list;
}

QList<BI_NetPoint*> Board::getNetPointsAtScenePos(const Point& pos, const BoardLayer* layer,
                                                  const NetSignal* netsignal) const noexcept
{
    QList<BI_NetPoint*> list;
    foreach (BI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->isSelectable() && netpoint->getGrabAreaScenePx().contains(pos.toPxQPointF())
            && ((!layer) || (&netpoint->getLayer() == layer))
            && ((!netsignal) || (&netpoint->getNetSignal() == netsignal)))
        {
            list.append(netpoint);
        }
    }
    return list;
}

QList<BI_NetLine*> Board::getNetLinesAtScenePos(const Point& pos, const BoardLayer* layer,
                                                const NetSignal* netsignal) const noexcept
{
    QList<BI_NetLine*> list;
    foreach (BI_NetLine* netline, mNetLines)
    {
        if (netline->isSelectable() && netline->getGrabAreaScenePx().contains(pos.toPxQPointF())
            && ((!layer) || (&netline->getLayer() == layer))
            && ((!netsignal) || (&netline->getNetSignal() == netsignal)))
        {
            list.append(netline);
        }
    }
    return list;
}

QList<BI_FootprintPad*> Board::getPadsAtScenePos(const Point& pos, const BoardLayer* layer,
                                                 const NetSignal* netsignal) const noexcept
{
    QList<BI_FootprintPad*> list;
    foreach (BI_Device* device, mDeviceInstances)
    {
        foreach (BI_FootprintPad* pad, device->getFootprint().getPads())
        {
            if (pad->isSelectable() && pad->getGrabAreaScenePx().contains(pos.toPxQPointF())
                && ((!layer) || (pad->isOnLayer(layer->getId())))
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
    foreach (BI_Via* via, mVias)
        items.append(via);
    foreach (BI_NetPoint* netpoint, mNetPoints)
        items.append(netpoint);
    foreach (BI_NetLine* netline, mNetLines)
        items.append(netline);
    foreach (BI_Polygon* polygon, mPolygons)
        items.append(polygon);
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
    instance.addToBoard(*mGraphicsScene); // can throw
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
    instance.removeFromBoard(*mGraphicsScene); // can throw
    mDeviceInstances.remove(instance.getComponentInstanceUuid());
    updateErcMessages();
    emit deviceRemoved(instance);
}

/*****************************************************************************************
 *  NetPoint Methods
 ****************************************************************************************/

BI_Via* Board::getViaByUuid(const Uuid& uuid) const noexcept
{
    foreach (BI_Via* via, mVias) {
        if (via->getUuid() == uuid)
            return via;
    }
    return nullptr;
}

void Board::addVia(BI_Via& via)
{
    if ((!mIsAddedToProject) || (mVias.contains(&via)) || (&via.getBoard() != this)) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no via with the same uuid in the list
    if (getViaByUuid(via.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a via with the UUID \"%1\"!"))
            .arg(via.getUuid().toStr()));
    }
    // add to board
    via.addToBoard(*mGraphicsScene); // can throw
    mVias.append(&via);
}

void Board::removeVia(BI_Via& via)
{
    if ((!mIsAddedToProject) || (!mVias.contains(&via))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    via.removeFromBoard(*mGraphicsScene); // can throw
    mVias.removeOne(&via);
}

/*****************************************************************************************
 *  NetPoint Methods
 ****************************************************************************************/

BI_NetPoint* Board::getNetPointByUuid(const Uuid& uuid) const noexcept
{
    foreach (BI_NetPoint* netpoint, mNetPoints) {
        if (netpoint->getUuid() == uuid)
            return netpoint;
    }
    return nullptr;
}

void Board::addNetPoint(BI_NetPoint& netpoint)
{
    if ((!mIsAddedToProject) || (mNetPoints.contains(&netpoint))
        || (&netpoint.getBoard() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netpoint with the same uuid in the list
    if (getNetPointByUuid(netpoint.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a netpoint with the UUID \"%1\"!"))
            .arg(netpoint.getUuid().toStr()));
    }
    // add to board
    netpoint.addToBoard(*mGraphicsScene); // can throw
    mNetPoints.append(&netpoint);
}

void Board::removeNetPoint(BI_NetPoint& netpoint)
{
    if ((!mIsAddedToProject) || (!mNetPoints.contains(&netpoint))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    netpoint.removeFromBoard(*mGraphicsScene); // can throw
    mNetPoints.removeOne(&netpoint);
}

/*****************************************************************************************
 *  NetLine Methods
 ****************************************************************************************/

BI_NetLine* Board::getNetLineByUuid(const Uuid& uuid) const noexcept
{
    foreach (BI_NetLine* netline, mNetLines) {
        if (netline->getUuid() == uuid)
            return netline;
    }
    return nullptr;
}

void Board::addNetLine(BI_NetLine& netline)
{
    if ((!mIsAddedToProject) || (mNetLines.contains(&netline))
        || (&netline.getBoard() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netline with the same uuid in the list
    if (getNetLineByUuid(netline.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a netline with the UUID \"%1\"!"))
            .arg(netline.getUuid().toStr()));
    }
    // add to board
    netline.addToBoard(*mGraphicsScene); // can throw
    mNetLines.append(&netline);
}

void Board::removeNetLine(BI_NetLine& netline)
{
    if ((!mIsAddedToProject) || (!mNetLines.contains(&netline))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    netline.removeFromBoard(*mGraphicsScene); // can throw
    mNetLines.removeOne(&netline);
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
    polygon.addToBoard(*mGraphicsScene); // can throw
    mPolygons.append(&polygon);
}

void Board::removePolygon(BI_Polygon& polygon)
{
    if ((!mIsAddedToProject) || (!mPolygons.contains(&polygon))) {
        throw LogicError(__FILE__, __LINE__);
    }
    polygon.removeFromBoard(*mGraphicsScene); // can throw
    mPolygons.removeOne(&polygon);
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
        item->addToBoard(*mGraphicsScene); // can throw
        sgl.add([this, item](){item->removeFromBoard(*mGraphicsScene);});
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
        item->removeFromBoard(*mGraphicsScene); // can throw
        sgl.add([this, item](){item->addToBoard(*mGraphicsScene);});
    }
    mIsAddedToProject = false;
    updateErcMessages();
    sgl.dismiss();
}

bool Board::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // save board XML file
    try
    {
        if (mIsAddedToProject)
        {
            DomDocument doc(*serializeToDomElement("board"));
            mXmlFile->save(doc, toOriginal);
        }
        else
        {
            mXmlFile->removeFile(toOriginal);
        }
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getMsg());
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
    if (updateItems)
    {
        QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
        foreach (BI_Device* component, mDeviceInstances)
        {
            BI_Footprint& footprint = component->getFootprint();
            bool selectFootprint = footprint.isSelectable() && footprint.getGrabAreaScenePx().intersects(rectPx);
            footprint.setSelected(selectFootprint);
            foreach (BI_FootprintPad* pad, footprint.getPads())
            {
                bool selectPad = pad->isSelectable() && pad->getGrabAreaScenePx().intersects(rectPx);
                pad->setSelected(selectFootprint || selectPad);
            }
        }
        foreach (BI_Via* via, mVias)
            via->setSelected(via->isSelectable() && via->getGrabAreaScenePx().intersects(rectPx));
        foreach (BI_NetPoint* netpoint, mNetPoints)
            netpoint->setSelected(netpoint->isSelectable() && netpoint->getGrabAreaScenePx().intersects(rectPx));
        foreach (BI_NetLine* netline, mNetLines)
            netline->setSelected(netline->isSelectable() && netline->getGrabAreaScenePx().intersects(rectPx));
    }
}

void Board::clearSelection() const noexcept
{
    foreach (BI_Device* device, mDeviceInstances)
        device->getFootprint().setSelected(false);
    foreach (BI_Via* via, mVias)
        via->setSelected(false);
    foreach (BI_NetPoint* netpoint, mNetPoints)
        netpoint->setSelected(false);
    foreach (BI_NetLine* netline, mNetLines)
        netline->setSelected(false);
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool Board::getAttributeValue(const QString& attrNS, const QString& attrKey,
                              bool passToParents, QString& value) const noexcept
{
    // TODO
    Q_UNUSED(attrNS);
    Q_UNUSED(attrKey);
    Q_UNUSED(passToParents);
    Q_UNUSED(value);
    return false;
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

void Board::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    // metadata
    root.appendTextChild("uuid", mUuid);
    root.appendTextChild("name", mName);
    // grid properties
    root.appendChild(mGridProperties->serializeToDomElement("grid"));
    // layer stack
    root.appendChild(mLayerStack->serializeToDomElement("layers"));
    // design rules
    root.appendChild(mDesignRules->serializeToDomElement("design_rules"));
    // devices
    serializePointerContainer(root, mDeviceInstances, "device");
    // vias
    serializePointerContainer(root, mVias, "via");
    // netpoints
    serializePointerContainer(root, mNetPoints, "netpoint");
    // netlines
    serializePointerContainer(root, mNetLines, "netline");
    // polygons
    serializePointerContainer(root, mPolygons, "polygon");
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
