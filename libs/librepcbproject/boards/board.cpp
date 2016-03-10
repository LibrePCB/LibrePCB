/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include "../project.h"
#include <librepcbcommon/graphics/graphicsview.h>
#include <librepcbcommon/graphics/graphicsscene.h>
#include <librepcbcommon/gridproperties.h>
#include "../circuit/circuit.h"
#include "../erc/ercmsg.h"
#include "../circuit/componentinstance.h"
#include "deviceinstance.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include <librepcblibrary/cmp/component.h>
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

Board::Board(Project& project, const FilePath& filepath, bool restore,
             bool readOnly, bool create, const QString& newName) throw (Exception):
    QObject(nullptr), IF_AttributeProvider(), mProject(project), mFilePath(filepath),
    mXmlFile(nullptr), mAddedToProject(false), mLayerStack(nullptr),
    mGraphicsScene(nullptr), mGridProperties(nullptr)
{
    try
    {
        mGraphicsScene = new GraphicsScene();

        // try to open/create the XML board file
        if (create)
        {
            mXmlFile = SmartXmlFile::create(mFilePath);

            // set attributes
            mUuid = Uuid::createRandom();
            mName = newName;

            // load default layer stack
            mLayerStack = new BoardLayerStack(*this);

            // load default grid properties
            mGridProperties = new GridProperties();
        }
        else
        {
            mXmlFile = new SmartXmlFile(mFilePath, restore, readOnly);
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree(true);
            XmlDomElement& root = doc->getRoot();

            // the board seems to be ready to open, so we will create all needed objects

            mUuid = root.getFirstChild("meta/uuid", true, true)->getText<Uuid>(true);
            mName = root.getFirstChild("meta/name", true, true)->getText<QString>(true);

            // Load layer stack
            mLayerStack = new BoardLayerStack(*this, *root.getFirstChild("layer_stack", true));

            // Load grid properties
            mGridProperties = new GridProperties(*root.getFirstChild("properties/grid_properties", true, true));

            // Load all device instances
            for (XmlDomElement* node = root.getFirstChild("devices/device", true, false);
                 node; node = node->getNextSibling("device"))
            {
                DeviceInstance* comp = new DeviceInstance(*this, *node);
                addDeviceInstance(*comp);
            }

            // Load all polygons
            for (XmlDomElement* node = root.getFirstChild("polygons/polygon", true, false);
                 node; node = node->getNextSibling("polygon"))
            {
                BI_Polygon* polygon = new BI_Polygon(*this, *node);
                addPolygon(*polygon);
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
        foreach (BI_Polygon* polygon, mPolygons)
            try { removePolygon(*polygon); delete polygon; } catch (...) {}
        foreach (DeviceInstance* dev, mDeviceInstances)
            try { removeDeviceInstance(*dev); delete dev; } catch (...) {}
        delete mGridProperties;         mGridProperties = nullptr;
        delete mLayerStack;             mLayerStack = nullptr;
        delete mXmlFile;                mXmlFile = nullptr;
        delete mGraphicsScene;          mGraphicsScene = nullptr;
        throw; // ...and rethrow the exception
    }
}

Board::~Board() noexcept
{
    // delete all component instances (and catch all throwed exceptions)
    foreach (BI_Polygon* polygon, mPolygons)
        try { removePolygon(*polygon); delete polygon; } catch (...) {}
    foreach (DeviceInstance* dev, mDeviceInstances)
        try { removeDeviceInstance(*dev); delete dev; } catch (...) {}

    qDeleteAll(mErcMsgListUnplacedComponentInstances);    mErcMsgListUnplacedComponentInstances.clear();
    delete mGridProperties;         mGridProperties = nullptr;
    delete mLayerStack;             mLayerStack = nullptr;
    delete mXmlFile;                mXmlFile = nullptr;
    delete mGraphicsScene;          mGraphicsScene = nullptr;
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

bool Board::isEmpty() const noexcept
{
    return false;
}

QList<BI_Base*> Board::getSelectedItems(bool footprintPads
                                        /*bool floatingPoints,
                                        bool attachedPoints,
                                        bool floatingPointsFromFloatingLines,
                                        bool attachedPointsFromFloatingLines,
                                        bool floatingPointsFromAttachedLines,
                                        bool attachedPointsFromAttachedLines,
                                        bool attachedPointsFromSymbols,
                                        bool floatingLines,
                                        bool attachedLines,
                                        bool attachedLinesFromFootprints*/) const noexcept
{
    QList<BI_Base*> list;
    foreach (DeviceInstance* component, mDeviceInstances)
    {
        BI_Footprint& footprint = component->getFootprint();

        // footprint
        if (footprint.isSelected())
            list.append(&footprint);

        // pads
        foreach (BI_FootprintPad* pad, footprint.getPads())
        {
            if (pad->isSelected() && footprintPads)
                list.append(pad);
        }
    }

    return list;
}

QList<BI_Base*> Board::getItemsAtScenePos(const Point& pos) const noexcept
{
    QPointF scenePosPx = pos.toPxQPointF();
    QList<BI_Base*> list;   // Note: The order of adding the items is very important (the
                            // top most item must appear as the first item in the list)!
    // footprints & pads
    foreach (DeviceInstance* device, mDeviceInstances)
    {
        BI_Footprint& footprint = device->getFootprint();
        if (footprint.getGrabAreaScenePx().contains(scenePosPx)) {
            if (footprint.getIsMirrored())
                list.append(&footprint);
            else
                list.prepend(&footprint);
        }
        foreach (BI_FootprintPad* pad, footprint.getPads())
        {
            if (pad->getGrabAreaScenePx().contains(scenePosPx)) {
                if (pad->getIsMirrored())
                    list.append(pad);
                else
                    list.insert(1, pad);
            }
        }
    }
    return list;
}

/*QList<SI_NetPoint*> Board::getNetPointsAtScenePos(const Point& pos) const noexcept
{
    QList<SI_NetPoint*> list;
    foreach (SI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->getGrabAreaScenePx().contains(pos.toPxQPointF()))
            list.append(netpoint);
    }
    return list;
}

QList<SI_NetLine*> Board::getNetLinesAtScenePos(const Point& pos) const noexcept
{
    QList<SI_NetLine*> list;
    foreach (SI_NetLine* netline, mNetLines)
    {
        if (netline->getGrabAreaScenePx().contains(pos.toPxQPointF()))
            list.append(netline);
    }
    return list;
}

QList<SI_SymbolPin*> Board::getPinsAtScenePos(const Point& pos) const noexcept
{
    QList<SI_SymbolPin*> list;
    foreach (SI_Symbol* symbol, mSymbols)
    {
        foreach (SI_SymbolPin* pin, symbol->getPins())
        {
            if (pin->getGrabAreaScenePx().contains(pos.toPxQPointF()))
                list.append(pin);
        }
    }
    return list;
}*/

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

DeviceInstance* Board::getDeviceInstanceByComponentUuid(const Uuid& uuid) const noexcept
{
    return mDeviceInstances.value(uuid, nullptr);
}

/*DeviceInstance* Board::createDeviceInstance() throw (Exception)
{
    if (getComponentInstanceByName(name))
    {
        throw RuntimeError(__FILE__, __LINE__, name, QString(tr("The component "
            "name \"%1\" does already exist in the circuit.")).arg(name));
    }
    return new ComponentInstance(*this, cmp, symbVar, name);
    return nullptr; // TODO
}*/

void Board::addDeviceInstance(DeviceInstance& instance) throw (Exception)
{
    // check if there is no device with the same component instance in the list
    if (getDeviceInstanceByComponentUuid(instance.getComponentInstance().getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, instance.getComponentInstance().getUuid().toStr(),
            QString(tr("There is already a device with the component instance \"%1\"!"))
            .arg(instance.getComponentInstance().getUuid().toStr()));
    }

    // add to circuit
    instance.addToBoard(*mGraphicsScene);
    mDeviceInstances.insert(instance.getComponentInstance().getUuid(), &instance);
    updateErcMessages();
    emit deviceAdded(instance);
}

void Board::removeDeviceInstance(DeviceInstance& instance) throw (Exception)
{
    Q_ASSERT(mDeviceInstances.contains(instance.getComponentInstance().getUuid()) == true);

    // remove from circuit
    instance.removeFromBoard(*mGraphicsScene);
    mDeviceInstances.remove(instance.getComponentInstance().getUuid());
    updateErcMessages();
    emit deviceRemoved(instance);
}

/*****************************************************************************************
 *  Polygon Methods
 ****************************************************************************************/

/*BI_Polygon* Board::createPolygon() throw (Exception)
{
    if (getComponentInstanceByName(name))
    {
        throw RuntimeError(__FILE__, __LINE__, name, QString(tr("The component "
            "name \"%1\" does already exist in the circuit.")).arg(name));
    }
    return new ComponentInstance(*this, cmp, symbVar, name);
    return nullptr; // TODO
}*/

void Board::addPolygon(BI_Polygon& polygon) throw (Exception)
{
    Q_ASSERT(!mPolygons.contains(&polygon));
    polygon.addToBoard(*mGraphicsScene);
    mPolygons.append(&polygon);
}

void Board::removePolygon(BI_Polygon& polygon) throw (Exception)
{
    Q_ASSERT(mPolygons.contains(&polygon));
    polygon.removeFromBoard(*mGraphicsScene);
    mPolygons.removeAll(&polygon);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Board::addToProject() throw (Exception)
{
    Q_ASSERT(mAddedToProject == false);
    mAddedToProject = true;
    updateErcMessages();
}

void Board::removeFromProject() throw (Exception)
{
    Q_ASSERT(mAddedToProject == true);
    mAddedToProject = false;
    updateErcMessages();
}

bool Board::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // save board XML file
    try
    {
        if (mAddedToProject)
        {
            XmlDomDocument doc(*serializeToXmlDomElement());
            doc.setFileVersion(APP_VERSION_MAJOR);
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
        errors.append(e.getUserMsg());
    }

    return success;
}

void Board::showInView(GraphicsView& view) noexcept
{
    view.setScene(mGraphicsScene);
}

void Board::setSelectionRect(const Point& p1, const Point& p2, bool updateItems) noexcept
{
    mGraphicsScene->setSelectionRect(p1, p2);
    if (updateItems)
    {
        QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
        foreach (DeviceInstance* component, mDeviceInstances)
        {
            BI_Footprint& footprint = component->getFootprint();
            bool selectFootprint = footprint.getGrabAreaScenePx().intersects(rectPx);
            footprint.setSelected(selectFootprint);
            foreach (BI_FootprintPad* pad, footprint.getPads())
            {
                bool selectPad = pad->getGrabAreaScenePx().intersects(rectPx);
                pad->setSelected(selectFootprint || selectPad);
            }
        }
    }
}

void Board::clearSelection() const noexcept
{
    foreach (DeviceInstance* device, mDeviceInstances)
        device->getFootprint().setSelected(false);
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

XmlDomElement* Board::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("board"));
    XmlDomElement* meta = root->appendChild("meta");
    meta->appendTextChild("uuid", mUuid);
    meta->appendTextChild("name", mName);
    XmlDomElement* properties = root->appendChild("properties");
    properties->appendChild(mGridProperties->serializeToXmlDomElement());
    root->appendChild(mLayerStack->serializeToXmlDomElement());
    XmlDomElement* device_instances = root->appendChild("devices");
    foreach (DeviceInstance* device, mDeviceInstances)
        device_instances->appendChild(device->serializeToXmlDomElement());
    XmlDomElement* polygons = root->appendChild("polygons");
    foreach (BI_Polygon* polygon, mPolygons)
        polygons->appendChild(polygon->serializeToXmlDomElement());
    return root.take();
}

void Board::updateErcMessages() noexcept
{
    // type: UnplacedComponent (ComponentInstances without DeviceInstance)
    if (mAddedToProject)
    {
        const QMap<Uuid, ComponentInstance*>& componentInstances = mProject.getCircuit().getComponentInstances();
        foreach (const ComponentInstance* component, componentInstances)
        {
            if (component->getLibComponent().isSchematicOnly()) continue;
            DeviceInstance* device = mDeviceInstances.value(component->getUuid());
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

Board* Board::create(Project& project, const FilePath& filepath,
                         const QString& name) throw (Exception)
{
    return new Board(project, filepath, false, false, true, name);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
