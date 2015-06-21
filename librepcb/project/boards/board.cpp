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
#include "componentinstance.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include <librepcblibrary/gencmp/genericcomponent.h>

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
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
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
                ComponentInstance* comp = new ComponentInstance(*this, *node);
                addComponentInstance(*comp);
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
        foreach (ComponentInstance* compInstance, mComponentInstances)
            try { removeComponentInstance(*compInstance); delete compInstance; } catch (...) {}
        delete mGridProperties;         mGridProperties = nullptr;
        delete mXmlFile;                mXmlFile = nullptr;
        delete mGraphicsScene;          mGraphicsScene = nullptr;
        throw; // ...and rethrow the exception
    }
}

Board::~Board()
{
    // delete all component instances (and catch all throwed exceptions)
    foreach (ComponentInstance* compInstance, mComponentInstances)
        try { removeComponentInstance(*compInstance); delete compInstance; } catch (...) {}

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

QList<BI_Base*> Board::getSelectedItems(/*bool floatingPoints,
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
    foreach (ComponentInstance* component, mComponentInstances)
    {
        BI_Footprint& footprint = component->getFootprint();

        // footprint
        if (footprint.isSelected())
            list.append(&footprint);

        // pads
        foreach (BI_FootprintPad* pad, footprint.getPads())
        {
            if (pad->isSelected())
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
    foreach (ComponentInstance* component, mComponentInstances)
    {
        BI_Footprint& footprint = component->getFootprint();
        foreach (BI_FootprintPad* pad, footprint.getPads())
        {
            if (pad->getGrabAreaScenePx().contains(scenePosPx))
                list.append(pad);
        }
        if (footprint.getGrabAreaScenePx().contains(scenePosPx))
            list.append(&footprint);
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
 *  ComponentInstance Methods
 ****************************************************************************************/

ComponentInstance* Board::getCompInstanceByGenCompUuid(const QUuid& uuid) const noexcept
{
    return mComponentInstances.value(uuid, nullptr);
}

ComponentInstance* Board::createComponentInstance() throw (Exception)
{
    /*if (getGenCompInstanceByName(name))
    {
        throw RuntimeError(__FILE__, __LINE__, name, QString(tr("The component "
            "name \"%1\" does already exist in the circuit.")).arg(name));
    }
    return new ComponentInstance(*this, genComp, symbVar, name);*/
    return nullptr; // TODO
}

void Board::addComponentInstance(ComponentInstance& componentInstance) throw (Exception)
{
    // check if there is no component with the same generic component instance in the list
    if (getCompInstanceByGenCompUuid(componentInstance.getGenCompInstance().getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, componentInstance.getGenCompInstance().getUuid().toString(),
            QString(tr("There is already a component with the generic component instance \"%1\"!"))
            .arg(componentInstance.getGenCompInstance().getUuid().toString()));
    }

    // add to circuit
    componentInstance.addToBoard(*mGraphicsScene);
    mComponentInstances.insert(componentInstance.getGenCompInstance().getUuid(), &componentInstance);
    updateErcMessages();
    emit componentAdded(componentInstance);
}

void Board::removeComponentInstance(ComponentInstance& componentInstance) throw (Exception)
{
    Q_ASSERT(mComponentInstances.contains(componentInstance.getGenCompInstance().getUuid()) == true);

    // remove from circuit
    componentInstance.removeFromBoard(*mGraphicsScene);
    mComponentInstances.remove(componentInstance.getGenCompInstance().getUuid());
    updateErcMessages();
    emit componentRemoved(componentInstance);
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
            XmlDomElement* root = serializeToXmlDomElement();
            XmlDomDocument doc(*root, true);
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
        foreach (ComponentInstance* component, mComponentInstances)
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
    foreach (ComponentInstance* component, mComponentInstances)
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
    foreach (ComponentInstance* component, mComponentInstances)
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
            ComponentInstance* comp = mComponentInstances.value(genComp->getUuid());
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
