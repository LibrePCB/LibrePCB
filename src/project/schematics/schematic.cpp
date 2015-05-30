/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "schematic.h"
#include "../../common/file_io/smartxmlfile.h"
#include "../../common/file_io/xmldomdocument.h"
#include "../../common/file_io/xmldomelement.h"
#include "../project.h"
#include "../../library/sym/symbolpin.h"
#include "items/si_symbol.h"
#include "items/si_symbolpin.h"
#include "items/si_netpoint.h"
#include "items/si_netline.h"
#include "items/si_netlabel.h"
#include "../../common/graphics/graphicsview.h"
#include "../../common/graphics/graphicsscene.h"
#include "../../common/gridproperties.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Schematic::Schematic(Project& project, const FilePath& filepath, bool restore,
                     bool readOnly, bool create, const QString& newName) throw (Exception):
    QObject(nullptr), IF_AttributeProvider(), mProject(project), mFilePath(filepath),
    mXmlFile(nullptr), mAddedToProject(false), mGraphicsScene(nullptr),
    mGridProperties(nullptr)
{
    try
    {
        mGraphicsScene = new GraphicsScene();

        // try to open/create the XML schematic file
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

            // the schematic seems to be ready to open, so we will create all needed objects

            mUuid = root.getFirstChild("meta/uuid", true, true)->getText<QUuid>();
            mName = root.getFirstChild("meta/name", true, true)->getText(true);

            // Load grid properties
            mGridProperties = new GridProperties(*root.getFirstChild("properties/grid_properties", true, true));

            // Load all symbols
            for (XmlDomElement* node = root.getFirstChild("symbols/symbol", true, false);
                 node; node = node->getNextSibling("symbol"))
            {
                SI_Symbol* symbol = new SI_Symbol(*this, *node);
                addSymbol(*symbol);
            }

            // Load all netpoints
            for (XmlDomElement* node = root.getFirstChild("netpoints/netpoint", true, false);
                 node; node = node->getNextSibling("netpoint"))
            {
                SI_NetPoint* netpoint = new SI_NetPoint(*this, *node);
                addNetPoint(*netpoint);
            }

            // Load all netlines
            for (XmlDomElement* node = root.getFirstChild("netlines/netline", true, false);
                 node; node = node->getNextSibling("netline"))
            {
                SI_NetLine* netline = new SI_NetLine(*this, *node);
                addNetLine(*netline);
            }

            // Load all netlabels
            for (XmlDomElement* node = root.getFirstChild("netlabels/netlabel", true, false);
                 node; node = node->getNextSibling("netlabel"))
            {
                SI_NetLabel* netlabel = new SI_NetLabel(*this, *node);
                addNetLabel(*netlabel);
            }
        }

        updateIcon();

        // emit the "attributesChanged" signal when the project has emited it
        connect(&mProject, &Project::attributesChanged, this, &Schematic::attributesChanged);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        foreach (SI_NetLabel* netlabel, mNetLabels)
            try { removeNetLabel(*netlabel); delete netlabel; } catch (...) {}
        foreach (SI_NetLine* netline, mNetLines)
            try { removeNetLine(*netline); delete netline; } catch (...) {}
        foreach (SI_NetPoint* netpoint, mNetPoints)
            try { removeNetPoint(*netpoint); delete netpoint; } catch (...) {}
        foreach (SI_Symbol* symbol, mSymbols)
            try { removeSymbol(*symbol); delete symbol; } catch (...) {}
        delete mGridProperties;         mGridProperties = nullptr;
        delete mXmlFile;                mXmlFile = nullptr;
        delete mGraphicsScene;          mGraphicsScene = nullptr;
        throw; // ...and rethrow the exception
    }
}

Schematic::~Schematic()
{
    // delete all netlabels (and catch all throwed exceptions)
    foreach (SI_NetLabel* netlabel, mNetLabels)
        try { removeNetLabel(*netlabel); delete netlabel; } catch (...) {}
    // delete all netlines (and catch all throwed exceptions)
    foreach (SI_NetLine* netline, mNetLines)
        try { removeNetLine(*netline); delete netline; } catch (...) {}
    // delete all netpoints (and catch all throwed exceptions)
    foreach (SI_NetPoint* netpoint, mNetPoints)
        try { removeNetPoint(*netpoint); delete netpoint; } catch (...) {}
    // delete all symbols (and catch all throwed exceptions)
    foreach (SI_Symbol* symbol, mSymbols)
        try { removeSymbol(*symbol); delete symbol; } catch (...) {}

    delete mGridProperties;         mGridProperties = nullptr;
    delete mXmlFile;                mXmlFile = nullptr;
    delete mGraphicsScene;          mGraphicsScene = nullptr;
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

bool Schematic::isEmpty() const noexcept
{
    return (mSymbols.isEmpty() && mNetPoints.isEmpty() && mNetLines.isEmpty());
}

QList<SI_Base*> Schematic::getSelectedItems(bool floatingPoints,
                                            bool attachedPoints,
                                            bool floatingPointsFromFloatingLines,
                                            bool attachedPointsFromFloatingLines,
                                            bool floatingPointsFromAttachedLines,
                                            bool attachedPointsFromAttachedLines,
                                            bool attachedPointsFromSymbols,
                                            bool floatingLines,
                                            bool attachedLines,
                                            bool attachedLinesFromSymbols) const noexcept
{
    QList<SI_Base*> list;
    foreach (SI_Symbol* symbol, mSymbols)
    {
        // symbol
        if (symbol->isSelected())
            list.append(symbol);

        // pins
        foreach (SI_SymbolPin* pin, symbol->getPins())
        {
            // pin
            if (pin->isSelected())
                list.append(pin);

            // attached netpoints & netlines
            SI_NetPoint* attachedNetPoint = pin->getNetPoint();
            if (symbol->isSelected() && attachedPointsFromSymbols && attachedNetPoint)
            {
                if (!list.contains(attachedNetPoint))
                    list.append(attachedNetPoint);
            }
            if (symbol->isSelected() && attachedLinesFromSymbols && attachedNetPoint)
            {
                foreach (SI_NetLine* attachedNetLine, attachedNetPoint->getLines())
                {
                    if (!list.contains(attachedNetLine))
                        list.append(attachedNetLine);
                }
            }
        }
    }
    foreach (SI_NetPoint* netpoint, mNetPoints)
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
    foreach (SI_NetLine* netline, mNetLines)
    {
        if (netline->isSelected())
        {
            // netline
            if (((!netline->isAttachedToSymbol()) && floatingLines)
               || (netline->isAttachedToSymbol() && attachedLines))
            {
                if (!list.contains(netline))
                    list.append(netline);
            }
            // netpoints from netlines
            SI_NetPoint* p1 = &netline->getStartPoint();
            SI_NetPoint* p2 = &netline->getEndPoint();
            if ( ((!netline->isAttachedToSymbol()) && (!p1->isAttached()) && floatingPointsFromFloatingLines)
              || ((!netline->isAttachedToSymbol()) && ( p1->isAttached()) && attachedPointsFromFloatingLines)
              || (( netline->isAttachedToSymbol()) && (!p1->isAttached()) && floatingPointsFromAttachedLines)
              || (( netline->isAttachedToSymbol()) && ( p1->isAttached()) && attachedPointsFromAttachedLines))
            {
                if (!list.contains(p1))
                    list.append(p1);
            }
            if ( ((!netline->isAttachedToSymbol()) && (!p2->isAttached()) && floatingPointsFromFloatingLines)
              || ((!netline->isAttachedToSymbol()) && ( p2->isAttached()) && attachedPointsFromFloatingLines)
              || (( netline->isAttachedToSymbol()) && (!p2->isAttached()) && floatingPointsFromAttachedLines)
              || (( netline->isAttachedToSymbol()) && ( p2->isAttached()) && attachedPointsFromAttachedLines))
            {
                if (!list.contains(p2))
                    list.append(p2);
            }
        }
    }
    foreach (SI_NetLabel* netlabel, mNetLabels)
    {
        if (netlabel->isSelected())
            list.append(netlabel);
    }

    return list;
}

QList<SI_Base*> Schematic::getItemsAtScenePos(const Point& pos) const noexcept
{
    QPointF scenePosPx = pos.toPxQPointF();
    QList<SI_Base*> list;   // Note: The order of adding the items is very important (the
                            // top most item must appear as the first item in the list)!
    // visible netpoints
    foreach (SI_NetPoint* netpoint, mNetPoints)
    {
        if (!netpoint->isVisible()) continue;
        if (netpoint->getGrabAreaScenePx().contains(scenePosPx))
            list.append(netpoint);
    }
    // hidden netpoints
    foreach (SI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->isVisible()) continue;
        if (netpoint->getGrabAreaScenePx().contains(scenePosPx))
            list.append(netpoint);
    }
    // netlines
    foreach (SI_NetLine* netline, mNetLines)
    {
        if (netline->getGrabAreaScenePx().contains(scenePosPx))
            list.append(netline);
    }
    // netlabels
    foreach (SI_NetLabel* netlabel, mNetLabels)
    {
        if (netlabel->getGrabAreaScenePx().contains(scenePosPx))
            list.append(netlabel);
    }
    // symbols & pins
    foreach (SI_Symbol* symbol, mSymbols)
    {
        foreach (SI_SymbolPin* pin, symbol->getPins())
        {
            if (pin->getGrabAreaScenePx().contains(scenePosPx))
                list.append(pin);
        }
        if (symbol->getGrabAreaScenePx().contains(scenePosPx))
            list.append(symbol);
    }
    return list;
}

QList<SI_NetPoint*> Schematic::getNetPointsAtScenePos(const Point& pos) const noexcept
{
    QList<SI_NetPoint*> list;
    foreach (SI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->getGrabAreaScenePx().contains(pos.toPxQPointF()))
            list.append(netpoint);
    }
    return list;
}

QList<SI_NetLine*> Schematic::getNetLinesAtScenePos(const Point& pos) const noexcept
{
    QList<SI_NetLine*> list;
    foreach (SI_NetLine* netline, mNetLines)
    {
        if (netline->getGrabAreaScenePx().contains(pos.toPxQPointF()))
            list.append(netline);
    }
    return list;
}

QList<SI_SymbolPin*> Schematic::getPinsAtScenePos(const Point& pos) const noexcept
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
}

/*****************************************************************************************
 *  Setters: General
 ****************************************************************************************/

void Schematic::setGridProperties(const GridProperties& grid) noexcept
{
    *mGridProperties = grid;
}

/*****************************************************************************************
 *  Symbol Methods
 ****************************************************************************************/

SI_Symbol* Schematic::getSymbolByUuid(const QUuid& uuid) const noexcept
{
    foreach (SI_Symbol* symbol, mSymbols)
    {
        if (symbol->getUuid() == uuid)
            return symbol;
    }
    return nullptr;
}

SI_Symbol* Schematic::createSymbol(GenCompInstance& genCompInstance, const QUuid& symbolItem,
                                   const Point& position, const Angle& angle) throw (Exception)
{
    return new SI_Symbol(*this, genCompInstance, symbolItem, position, angle);
}

void Schematic::addSymbol(SI_Symbol& symbol) throw (Exception)
{
    // check if there is no symbol with the same uuid in the list
    if (getSymbolByUuid(symbol.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, symbol.getUuid().toString(),
            QString(tr("There is already a symbol with the UUID \"%1\"!"))
            .arg(symbol.getUuid().toString()));
    }

    // add to schematic
    symbol.addToSchematic(*mGraphicsScene); // can throw an exception
    mSymbols.append(&symbol);
}

void Schematic::removeSymbol(SI_Symbol& symbol) throw (Exception)
{
    Q_ASSERT(mSymbols.contains(&symbol) == true);

    // remove from schematic
    symbol.removeFromSchematic(*mGraphicsScene); // can throw an exception
    mSymbols.removeAll(&symbol);
}

/*****************************************************************************************
 *  NetPoint Methods
 ****************************************************************************************/

SI_NetPoint* Schematic::getNetPointByUuid(const QUuid& uuid) const noexcept
{
    foreach (SI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->getUuid() == uuid)
            return netpoint;
    }
    return nullptr;
}

SI_NetPoint* Schematic::createNetPoint(NetSignal& netsignal, const Point& position) throw (Exception)
{
    return new SI_NetPoint(*this, netsignal, position);
}

SI_NetPoint* Schematic::createNetPoint(SI_SymbolPin& pin) throw (Exception)
{
    return new SI_NetPoint(*this, pin);
}

void Schematic::addNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    // check if there is no netpoint with the same uuid in the list
    if (getNetPointByUuid(netpoint.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netpoint.getUuid().toString(),
            QString(tr("There is already a netpoint with the UUID \"%1\"!"))
            .arg(netpoint.getUuid().toString()));
    }

    // add to schematic
    netpoint.addToSchematic(*mGraphicsScene); // can throw an exception
    mNetPoints.append(&netpoint);
}

void Schematic::removeNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    Q_ASSERT(mNetPoints.contains(&netpoint) == true);

    // the netpoint cannot be removed if there are already netlines connected to it!
    if (netpoint.getLines().count() > 0)
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
            .arg(netpoint.getUuid().toString()).arg(netpoint.getLines().count()),
            QString(tr("There are already netlines connected to the netpoint \"%1\"!"))
            .arg(netpoint.getUuid().toString()));
    }

    // remove from schematic
    netpoint.removeFromSchematic(*mGraphicsScene); // can throw an exception
    mNetPoints.removeAll(&netpoint);
}

/*****************************************************************************************
 *  NetLine Methods
 ****************************************************************************************/

SI_NetLine* Schematic::getNetLineByUuid(const QUuid& uuid) const noexcept
{
    foreach (SI_NetLine* netline, mNetLines)
    {
        if (netline->getUuid() == uuid)
            return netline;
    }
    return nullptr;
}

SI_NetLine* Schematic::createNetLine(SI_NetPoint& startPoint, SI_NetPoint& endPoint,
                                     const Length& width) throw (Exception)
{
    return new SI_NetLine(*this, startPoint, endPoint, width);
}

void Schematic::addNetLine(SI_NetLine& netline) throw (Exception)
{
    // check if there is no netline with the same uuid in the list
    if (getNetLineByUuid(netline.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netline.getUuid().toString(),
            QString(tr("There is already a netline with the UUID \"%1\"!"))
            .arg(netline.getUuid().toString()));
    }

    // add to schematic
    netline.addToSchematic(*mGraphicsScene); // can throw an exception
    mNetLines.append(&netline);
}

void Schematic::removeNetLine(SI_NetLine& netline) throw (Exception)
{
    Q_ASSERT(mNetLines.contains(&netline) == true);

    // remove from schematic
    netline.removeFromSchematic(*mGraphicsScene); // can throw an exception
    mNetLines.removeAll(&netline);
}

/*****************************************************************************************
 *  NetLabel Methods
 ****************************************************************************************/

SI_NetLabel* Schematic::getNetLabelByUuid(const QUuid& uuid) const noexcept
{
    foreach (SI_NetLabel* netlabel, mNetLabels)
    {
        if (netlabel->getUuid() == uuid)
            return netlabel;
    }
    return nullptr;
}

SI_NetLabel* Schematic::createNetLabel(NetSignal& netsignal, const Point& position) throw (Exception)
{
    return new SI_NetLabel(*this, netsignal, position);
}

void Schematic::addNetLabel(SI_NetLabel& netlabel) throw (Exception)
{
    // check if there is no netlabel with the same uuid in the list
    if (getNetLabelByUuid(netlabel.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netlabel.getUuid().toString(),
            QString(tr("There is already a netlabel with the UUID \"%1\"!"))
            .arg(netlabel.getUuid().toString()));
    }

    // add to schematic
    netlabel.addToSchematic(*mGraphicsScene); // can throw an exception
    mNetLabels.append(&netlabel);
}

void Schematic::removeNetLabel(SI_NetLabel& netlabel) throw (Exception)
{
    Q_ASSERT(mNetLabels.contains(&netlabel) == true);

    // remove from schematic
    netlabel.removeFromSchematic(*mGraphicsScene); // can throw an exception
    mNetLabels.removeAll(&netlabel);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Schematic::addToProject() throw (Exception)
{
    Q_ASSERT(mAddedToProject == false);
    mAddedToProject = true;
}

void Schematic::removeFromProject() throw (Exception)
{
    Q_ASSERT(mAddedToProject == true);
    mAddedToProject = false;
}

bool Schematic::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // save schematic XML file
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

void Schematic::showInView(GraphicsView& view) noexcept
{
    view.setScene(mGraphicsScene);
}

void Schematic::setSelectionRect(const Point& p1, const Point& p2, bool updateItems) noexcept
{
    mGraphicsScene->setSelectionRect(p1, p2);
    if (updateItems)
    {
        QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
        foreach (SI_Symbol* symbol, mSymbols)
        {
            bool selectSymbol = symbol->getGrabAreaScenePx().intersects(rectPx);
            symbol->setSelected(selectSymbol);
            foreach (SI_SymbolPin* pin, symbol->getPins())
            {
                bool selectPin = pin->getGrabAreaScenePx().intersects(rectPx);
                pin->setSelected(selectSymbol || selectPin);
            }
        }
        foreach (SI_NetPoint* netpoint, mNetPoints)
            netpoint->setSelected(netpoint->getGrabAreaScenePx().intersects(rectPx));
        foreach (SI_NetLine* netline, mNetLines)
            netline->setSelected(netline->getGrabAreaScenePx().intersects(rectPx));
        foreach (SI_NetLabel* netlabel, mNetLabels)
            netlabel->setSelected(netlabel->getGrabAreaScenePx().intersects(rectPx));
    }
}

void Schematic::clearSelection() const noexcept
{
    foreach (SI_Symbol* symbol, mSymbols)
        symbol->setSelected(false);
    foreach (SI_NetPoint* netpoint, mNetPoints)
        netpoint->setSelected(false);
    foreach (SI_NetLine* netline, mNetLines)
        netline->setSelected(false);
    foreach (SI_NetLabel* netlabel, mNetLabels)
        netlabel->setSelected(false);
}

void Schematic::renderToQPainter(QPainter& painter) const noexcept
{
    GraphicsScene* scene = dynamic_cast<GraphicsScene*>(mGraphicsScene);
    Q_ASSERT(scene);
    scene->render(&painter, QRectF(), scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool Schematic::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                  bool passToParents, QString& value) const noexcept
{
    if ((attrNS == QLatin1String("PAGE")) || (attrNS.isEmpty()))
    {
        if (attrKey == QLatin1String("NAME"))
            return value = mName, true;
        else if (attrKey == QLatin1String("AUTHOR"))
            return value = mProject.getAuthor(), true;
        else if (attrKey == QLatin1String("CREATED"))
            return value = mProject.getCreated().toString(Qt::SystemLocaleShortDate), true;
        else if (attrKey == QLatin1String("LAST_MODIFIED"))
            return value = mProject.getLastModified().toString(Qt::SystemLocaleShortDate), true;
        else if (attrKey == QLatin1String("NBR"))
            return value = QString::number(mProject.getSchematicIndex(this) + 1), true;
        else if (attrKey == QLatin1String("CNT"))
            return value = QString::number(mProject.getSchematics().count()), true;
    }

    if ((attrNS != QLatin1String("PAGE")) && (passToParents))
        return mProject.getAttributeValue(attrNS, attrKey, passToParents, value);
    else
        return false;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Schematic::updateIcon() noexcept
{
    QRectF source = mGraphicsScene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
    QRect target(0, 0, 297, 210); // DIN A4 format :-)

    QPixmap pixmap(target.size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    mGraphicsScene->render(&painter, target, source);
    mIcon = QIcon(pixmap);
}

bool Schematic::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mName.isEmpty())    return false;
    return true;
}

XmlDomElement* Schematic::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("schematic"));
    XmlDomElement* meta = root->appendChild("meta");
    meta->appendTextChild("uuid", mUuid);
    meta->appendTextChild("name", mName);
    XmlDomElement* properties = root->appendChild("properties");
    properties->appendChild(mGridProperties->serializeToXmlDomElement());
    XmlDomElement* symbols = root->appendChild("symbols");
    foreach (SI_Symbol* symbolInstance, mSymbols)
        symbols->appendChild(symbolInstance->serializeToXmlDomElement());
    XmlDomElement* netpoints = root->appendChild("netpoints");
    foreach (SI_NetPoint* netpoint, mNetPoints)
        netpoints->appendChild(netpoint->serializeToXmlDomElement());
    XmlDomElement* netlines = root->appendChild("netlines");
    foreach (SI_NetLine* netline, mNetLines)
        netlines->appendChild(netline->serializeToXmlDomElement());
    XmlDomElement* netlabels = root->appendChild("netlabels");
    foreach (SI_NetLabel* netlabel, mNetLabels)
        netlabels->appendChild(netlabel->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Schematic* Schematic::create(Project& project, const FilePath& filepath,
                             const QString& name) throw (Exception)
{
    return new Schematic(project, filepath, false, false, true, name);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
