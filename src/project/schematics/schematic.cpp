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
#include "../../common/smartxmlfile.h"
#include "../../common/file_io/xmldomdocument.h"
#include "../../common/file_io/xmldomelement.h"
#include "../project.h"
#include "symbolinstance.h"
#include "schematicnetpoint.h"
#include "schematicnetline.h"
#include "../../library/symbolpingraphicsitem.h"
#include "../../library/symbolpin.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Schematic::Schematic(Project& project, const FilePath& filepath, bool restore,
                     bool readOnly, bool create, const QString& newName) throw (Exception):
    CADScene(), IF_AttributeProvider(), mProject(project), mFilePath(filepath),
    mXmlFile(nullptr), mAddedToProject(false)
{
    try
    {
        // try to open/create the XML schematic file
        if (create)
        {
            mXmlFile = SmartXmlFile::create(mFilePath);

            // set attributes
            mUuid = QUuid::createUuid();
            mName = newName;
        }
        else
        {
            mXmlFile = new SmartXmlFile(mFilePath, restore, readOnly);
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
            XmlDomElement& root = doc->getRoot();

            // the schematic seems to be ready to open, so we will create all needed objects

            mUuid = root.getFirstChild("meta/uuid", true, true)->getText<QUuid>();
            mName = root.getFirstChild("meta/name", true, true)->getText(true);

            // Load all symbol instances
            for (XmlDomElement* node = root.getFirstChild("symbols/symbol", true, false);
                 node; node = node->getNextSibling("symbol"))
            {
                SymbolInstance* symbol = new SymbolInstance(*this, *node);
                addSymbol(*symbol);
            }

            // Load all netpoints
            for (XmlDomElement* node = root.getFirstChild("netpoints/netpoint", true, false);
                 node; node = node->getNextSibling("netpoint"))
            {
                SchematicNetPoint* netpoint = new SchematicNetPoint(*this, *node);
                addNetPoint(*netpoint);
            }

            // Load all netlines
            for (XmlDomElement* node = root.getFirstChild("netlines/netline", true, false);
                 node; node = node->getNextSibling("netline"))
            {
                SchematicNetLine* netline = new SchematicNetLine(*this, *node);
                addNetLine(*netline);
            }
        }

        updateIcon();

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        foreach (SchematicNetLine* netline, mNetLines)
            try { removeNetLine(*netline); delete netline; } catch (...) {}
        foreach (SchematicNetPoint* netpoint, mNetPoints)
            try { removeNetPoint(*netpoint); delete netpoint; } catch (...) {}
        foreach (SymbolInstance* symbol, mSymbols)
            try { removeSymbol(*symbol); delete symbol; } catch (...) {}
        delete mXmlFile;                mXmlFile = 0;
        throw; // ...and rethrow the exception
    }
}

Schematic::~Schematic()
{
    // delete all netlines (and catch all throwed exceptions)
    foreach (SchematicNetLine* netline, mNetLines)
        try { removeNetLine(*netline); delete netline; } catch (...) {}
    // delete all netpoints (and catch all throwed exceptions)
    foreach (SchematicNetPoint* netpoint, mNetPoints)
        try { removeNetPoint(*netpoint); delete netpoint; } catch (...) {}
    // delete all symbols (and catch all throwed exceptions)
    foreach (SymbolInstance* symbol, mSymbols)
        try { removeSymbol(*symbol); delete symbol; } catch (...) {}

    delete mXmlFile;                mXmlFile = 0;
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

bool Schematic::isEmpty() const noexcept
{
    return (mSymbols.isEmpty() && mNetPoints.isEmpty() && mNetLines.isEmpty());
}

uint Schematic::getNetPointsAtScenePos(QList<SchematicNetPoint*>& list, const Point& pos) const noexcept
{
    foreach (QGraphicsItem* i, items(pos.toPxQPointF()))
    {
        if (i->type() != CADScene::Type_SchematicNetPoint) continue;
        SchematicNetPointGraphicsItem* p = qgraphicsitem_cast<SchematicNetPointGraphicsItem*>(i);
        if (!p) continue;
        list.append(&p->getNetPoint());
    }
    return list.count();
}

uint Schematic::getNetLinesAtScenePos(QList<SchematicNetLine*>& list, const Point& pos) const noexcept
{
    foreach (QGraphicsItem* i, items(pos.toPxQPointF()))
    {
        if (i->type() != CADScene::Type_SchematicNetLine) continue;
        SchematicNetLineGraphicsItem* l = qgraphicsitem_cast<SchematicNetLineGraphicsItem*>(i);
        if (!l) continue;
        list.append(&l->getNetLine());
    }
    return list.count();
}

uint Schematic::getPinsAtScenePos(QList<SymbolPinInstance*>& list, const Point& pos) const noexcept
{
    foreach (QGraphicsItem* i, items(pos.toPxQPointF()))
    {
        if (i->type() != CADScene::Type_SymbolPin) continue;
        library::SymbolPinGraphicsItem* p = qgraphicsitem_cast<library::SymbolPinGraphicsItem*>(i);
        if (!p) continue;
        list.append(p->getPinInstance());
    }
    return list.count();
}

/*****************************************************************************************
 *  SymbolInstance Methods
 ****************************************************************************************/

SymbolInstance* Schematic::getSymbolByUuid(const QUuid& uuid) const noexcept
{
    return mSymbols.value(uuid, 0);
}

SymbolInstance* Schematic::createSymbol(GenCompInstance& genCompInstance, const QUuid& symbolItem,
                                        const Point& position, const Angle& angle) throw (Exception)
{
    return new SymbolInstance(*this, genCompInstance, symbolItem, position, angle);
}

void Schematic::addSymbol(SymbolInstance& symbol) throw (Exception)
{
    // check if there is no symbol with the same uuid in the list
    if (getSymbolByUuid(symbol.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, symbol.getUuid().toString(),
            QString(tr("There is already a symbol with the UUID \"%1\"!"))
            .arg(symbol.getUuid().toString()));
    }

    // add to schematic
    symbol.addToSchematic(); // can throw an exception
    mSymbols.insert(symbol.getUuid(), &symbol);
}

void Schematic::removeSymbol(SymbolInstance& symbol) throw (Exception)
{
    Q_ASSERT(mSymbols.contains(symbol.getUuid()) == true);

    // the symbol cannot be removed if there are already netpoints connected to it!
    /*if (symbol->getNetPointCount() > 0)
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
            .arg(symbol->getUuid().toString()).arg(symbol->getNetPointCount()),
            QString(tr("There are already netpoints connected to the symbol \"%1\"!"))
            .arg(symbol->getUuid().toString()));
    }*/

    // remove from schematic
    symbol.removeFromSchematic(); // can throw an exception
    mSymbols.remove(symbol.getUuid());
}

/*****************************************************************************************
 *  SchematicNetPoint Methods
 ****************************************************************************************/

SchematicNetPoint* Schematic::getNetPointByUuid(const QUuid& uuid) const noexcept
{
    return mNetPoints.value(uuid, 0);
}

SchematicNetPoint* Schematic::createNetPoint(NetSignal& netsignal, const Point& position) throw (Exception)
{
    return new SchematicNetPoint(*this, netsignal, position);
}

SchematicNetPoint* Schematic::createNetPoint(SymbolInstance& symbol, const QUuid& pin) throw (Exception)
{
    return new SchematicNetPoint(*this, symbol, pin);
}

void Schematic::addNetPoint(SchematicNetPoint& netpoint) throw (Exception)
{
    // check if there is no netpoint with the same uuid in the list
    if (getNetPointByUuid(netpoint.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netpoint.getUuid().toString(),
            QString(tr("There is already a netpoint with the UUID \"%1\"!"))
            .arg(netpoint.getUuid().toString()));
    }

    // add to schematic
    netpoint.addToSchematic(); // can throw an exception
    mNetPoints.insert(netpoint.getUuid(), &netpoint);
}

void Schematic::removeNetPoint(SchematicNetPoint& netpoint) throw (Exception)
{
    Q_ASSERT(mNetPoints.contains(netpoint.getUuid()) == true);

    // the netpoint cannot be removed if there are already netlines connected to it!
    if (netpoint.getLines().count() > 0)
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
            .arg(netpoint.getUuid().toString()).arg(netpoint.getLines().count()),
            QString(tr("There are already netlines connected to the netpoint \"%1\"!"))
            .arg(netpoint.getUuid().toString()));
    }

    // remove from schematic
    netpoint.removeFromSchematic(); // can throw an exception
    mNetPoints.remove(netpoint.getUuid());
}

/*****************************************************************************************
 *  SchematicNetLine Methods
 ****************************************************************************************/

SchematicNetLine* Schematic::getNetLineByUuid(const QUuid& uuid) const noexcept
{
    return mNetLines.value(uuid, 0);
}

SchematicNetLine* Schematic::createNetLine(SchematicNetPoint& startPoint,
                                           SchematicNetPoint& endPoint,
                                           const Length& width) throw (Exception)
{
    return new SchematicNetLine(*this, startPoint, endPoint, width);
}

void Schematic::addNetLine(SchematicNetLine& netline) throw (Exception)
{
    // check if there is no netline with the same uuid in the list
    if (getNetLineByUuid(netline.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netline.getUuid().toString(),
            QString(tr("There is already a netline with the UUID \"%1\"!"))
            .arg(netline.getUuid().toString()));
    }

    // add to schematic
    netline.addToSchematic(); // can throw an exception
    mNetLines.insert(netline.getUuid(), &netline);
}

void Schematic::removeNetLine(SchematicNetLine& netline) throw (Exception)
{
    Q_ASSERT(mNetLines.contains(netline.getUuid()) == true);

    // remove from schematic
    netline.removeFromSchematic(); // can throw an exception
    mNetLines.remove(netline.getUuid());
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
            return value = QString::number(mProject.getSchematicCount()), true;
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
    QRectF source = itemsBoundingRect().adjusted(-20, -20, 20, 20);
    QRect target(0, 0, 297, 210); // DIN A4 format :-)

    QPixmap pixmap(target.size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    render(&painter, target, source);
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
    XmlDomElement* symbols = root->appendChild("symbols");
    foreach (SymbolInstance* symbolInstance, mSymbols)
        symbols->appendChild(symbolInstance->serializeToXmlDomElement());
    XmlDomElement* netpoints = root->appendChild("netpoints");
    foreach (SchematicNetPoint* netpoint, mNetPoints)
        netpoints->appendChild(netpoint->serializeToXmlDomElement());
    XmlDomElement* netlines = root->appendChild("netlines");
    foreach (SchematicNetLine* netline, mNetLines)
        netlines->appendChild(netline->serializeToXmlDomElement());
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
