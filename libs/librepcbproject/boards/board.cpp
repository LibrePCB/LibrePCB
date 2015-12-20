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
#include "../circuit/gencompinstance.h"
#include "deviceinstance.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include <librepcblibrary/cmp/component.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Board::Board(Project& project, const FilePath& filepath, bool restore,
             bool readOnly, bool create, const QString& newName) throw (Exception):
    QObject(nullptr), IF_AttributeProvider(), mProject(project), mFilePath(filepath),
    mXmlFile(nullptr), mAddedToProject(false), mGraphicsScene(nullptr),
    mGridProperties(nullptr)
{
    try
    {
        mGraphicsScene = new GraphicsScene();

        // try to open/create the XML board file
        if (create)
        {
            mXmlFile = SmartXmlFile::create(mFilePath);

            // set attributes
            mUuid = QUuid::createUuid();
            mName = newName;

            // load default grid properties
            mGridProperties = new GridProperties();
        }
        else
        {
            mXmlFile = new SmartXmlFile(mFilePath, restore, readOnly);
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree(true);
            XmlDomElement& root = doc->getRoot();

            // the board seems to be ready to open, so we will create all needed objects

            mUuid = root.getFirstChild("meta/uuid", true, true)->getText<QUuid>();
            mName = root.getFirstChild("meta/name", true, true)->getText(true);

            // Load grid properties
            mGridProperties = new GridProperties(*root.getFirstChild("properties/grid_properties", true, true));

            // Load all component instances
            for (XmlDomElement* node = root.getFirstChild("component_instances/component_instance", true, false);
                 node; node = node->getNextSibling("component_instance"))
            {
                DeviceInstance* comp = new DeviceInstance(*this, *node);
                addDeviceInstance(*comp);
            }
        }

        updateErcMessages();
        updateIcon();

        // emit the "attributesChanged" signal when the project has emited it
        connect(&mProject, &Project::attributesChanged, this, &Board::attributesChanged);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        foreach (DeviceInstance* dev, mDeviceInstances)
            try { removeDeviceInstance(*dev); delete dev; } catch (...) {}
        delete mGridProperties;         mGridProperties = nullptr;
        delete mXmlFile;                mXmlFile = nullptr;
        delete mGraphicsScene;          mGraphicsScene = nullptr;
        throw; // ...and rethrow the exception
    }
}

Board::~Board() noexcept
{
    // delete all component instances (and catch all throwed exceptions)
    foreach (DeviceInstance* dev, mDeviceInstances)
        try { removeDeviceInstance(*dev); delete dev; } catch (...) {}

    qDeleteAll(mErcMsgListUnplacedGenCompInstances);    mErcMsgListUnplacedGenCompInstances.clear();
    delete mGridProperties;         mGridProperties = nullptr;
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
    foreach (DeviceInstance* component, mDeviceInstances)
    {
        BI_Footprint& footprint = component->getFootprint();
        if (footprint.getGrabAreaScenePx().contains(scenePosPx))
            list.append(&footprint);
        foreach (BI_FootprintPad* pad, footprint.getPads())
        {
            if (pad->getGrabAreaScenePx().contains(scenePosPx))
                list.append(pad);
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

DeviceInstance* Board::getDeviceInstanceByComponentUuid(const QUuid& uuid) const noexcept
{
    return mDeviceInstances.value(uuid, nullptr);
}

DeviceInstance* Board::createDeviceInstance() throw (Exception)
{
    /*if (getGenCompInstanceByName(name))
    {
        throw RuntimeError(__FILE__, __LINE__, name, QString(tr("The component "
            "name \"%1\" does already exist in the circuit.")).arg(name));
    }
    return new ComponentInstance(*this, genComp, symbVar, name);*/
    return nullptr; // TODO
}

void Board::addDeviceInstance(DeviceInstance& instance) throw (Exception)
{
    // check if there is no device with the same component instance in the list
    if (getDeviceInstanceByComponentUuid(instance.getComponentInstance().getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, instance.getComponentInstance().getUuid().toString(),
            QString(tr("There is already a device with the component instance \"%1\"!"))
            .arg(instance.getComponentInstance().getUuid().toString()));
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
    foreach (DeviceInstance* component, mDeviceInstances)
        component->getFootprint().setSelected(false);
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
    XmlDomElement* components = root->appendChild("component_instances");
    foreach (DeviceInstance* component, mDeviceInstances)
        components->appendChild(component->serializeToXmlDomElement());
    return root.take();
}

void Board::updateErcMessages() noexcept
{
    // type: UnplacedGenericComponent (GenCompInstances without ComponentInstance)
    if (mAddedToProject)
    {
        foreach (const GenCompInstance* genComp, mProject.getCircuit().getGenCompInstances())
        {
            if (genComp->getGenComp().isSchematicOnly()) continue;
            DeviceInstance* comp = mDeviceInstances.value(genComp->getUuid());
            ErcMsg* ercMsg = mErcMsgListUnplacedGenCompInstances.value(genComp->getUuid());
            if ((!comp) && (!ercMsg))
            {
                ErcMsg* ercMsg = new ErcMsg(mProject, *this, QString("%1/%2").arg(mUuid.toString(),
                    genComp->getUuid().toString()), "UnplacedGenericComponent", ErcMsg::ErcMsgType_t::BoardError,
                    QString("Unplaced Component: %1 (Board: %2)").arg(genComp->getName(), mName));
                ercMsg->setVisible(true);
                mErcMsgListUnplacedGenCompInstances.insert(genComp->getUuid(), ercMsg);
            }
            else if ((comp) && (ercMsg))
            {
                delete mErcMsgListUnplacedGenCompInstances.take(genComp->getUuid());
            }
        }
    }
    else
    {
        qDeleteAll(mErcMsgListUnplacedGenCompInstances);
        mErcMsgListUnplacedGenCompInstances.clear();
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
