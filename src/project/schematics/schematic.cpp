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
#include "../../common/units.h"
#include "../../common/xmlfile.h"
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
                     bool readOnly, bool isNew)
                     throw (Exception):
    CADScene(), mProject(project), mFilePath(filepath), mXmlFile(0)
{
    try
    {
        // try to open the XML schematic file
        mXmlFile = new XmlFile(mFilePath, restore, readOnly, QStringLiteral("schematic"));

        // the schematic seems to be ready to open, so we will create all needed objects

        QDomElement tmpNode; // for temporary use

        tmpNode = mXmlFile->getRoot().firstChildElement("meta");
        mUuid = tmpNode.firstChildElement("uuid").text();
        if(mUuid.isNull())
        {
            throw RuntimeError(__FILE__, __LINE__, tmpNode.firstChildElement("uuid").text(),
                QString(tr("Invalid schematic UUID: \"%1\""))
                .arg(tmpNode.firstChildElement("uuid").text()));
        }

        mName = tmpNode.firstChildElement("name").text();
        if(mName.isEmpty())
        {
            throw RuntimeError(__FILE__, __LINE__, mUuid.toString(),
                QString(tr("Name of schematic \"%1\" is empty!")).arg(mUuid.toString()));
        }

        // Load all symbol instances
        tmpNode = mXmlFile->getRoot().firstChildElement("symbols").firstChildElement("symbol");
        while (!tmpNode.isNull())
        {
            SymbolInstance* symbol = new SymbolInstance(*this, tmpNode);
            addSymbol(symbol, false);
            tmpNode = tmpNode.nextSiblingElement("symbol");
        }

        // Load all netpoints
        if (isNew)
        {
            mXmlFile->getRoot().appendChild(mXmlFile->getDocument().createElement("netpoints"));
        }
        else
        {
            tmpNode = mXmlFile->getRoot().firstChildElement("netpoints").firstChildElement("netpoint");
            while (!tmpNode.isNull())
            {
                SchematicNetPoint* netpoint = new SchematicNetPoint(*this, tmpNode);
                addNetPoint(netpoint, false);
                tmpNode = tmpNode.nextSiblingElement("netpoint");
            }
        }

        // Load all netlines
        if (isNew)
        {
            mXmlFile->getRoot().appendChild(mXmlFile->getDocument().createElement("netlines"));
        }
        else
        {
            tmpNode = mXmlFile->getRoot().firstChildElement("netlines").firstChildElement("netline");
            while (!tmpNode.isNull())
            {
                SchematicNetLine* netline = new SchematicNetLine(*this, tmpNode);
                addNetLine(netline, false);
                tmpNode = tmpNode.nextSiblingElement("netline");
            }
        }

        updateIcon();
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        foreach (SchematicNetLine* netline, mNetLines)
            try { removeNetLine(netline, false, true); } catch (...) {}
        foreach (SchematicNetPoint* netpoint, mNetPoints)
            try { removeNetPoint(netpoint, false, true); } catch (...) {}
        foreach (SymbolInstance* symbol, mSymbols)
            try { removeSymbol(symbol, false, true); } catch (...) {}
        delete mXmlFile;                mXmlFile = 0;
        throw; // ...and rethrow the exception
    }
}

Schematic::~Schematic()
{
    // delete all netlines (and catch all throwed exceptions)
    foreach (SchematicNetLine* netline, mNetLines)
        try { removeNetLine(netline, false, true); } catch (...) {}
    // delete all netpoints (and catch all throwed exceptions)
    foreach (SchematicNetPoint* netpoint, mNetPoints)
        try { removeNetPoint(netpoint, false, true); } catch (...) {}
    // delete all symbols (and catch all throwed exceptions)
    foreach (SymbolInstance* symbol, mSymbols)
        try { removeSymbol(symbol, false, true); } catch (...) {}

    delete mXmlFile;                mXmlFile = 0;
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

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

SymbolInstance* Schematic::createSymbol(const QUuid& genCompInstance, const QUuid& symbolItem,
                                        const Point& position, const Angle& angle,
                                        bool mirror) throw (Exception)
{
    return SymbolInstance::create(*this, mXmlFile->getDocument(), genCompInstance,
                                  symbolItem, position, angle, mirror);
}

void Schematic::addSymbol(SymbolInstance* symbol, bool toDomTree) throw (Exception)
{
    Q_CHECK_PTR(symbol);

    // check if there is no symbol with the same uuid in the list
    if (getSymbolByUuid(symbol->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, symbol->getUuid().toString(),
            QString(tr("There is already a symbol with the UUID \"%1\"!"))
            .arg(symbol->getUuid().toString()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("symbols");
    symbol->addToSchematic(*this, toDomTree, parent); // can throw an exception

    mSymbols.insert(symbol->getUuid(), symbol);
}

void Schematic::removeSymbol(SymbolInstance* symbol, bool fromDomTree,
                             bool deleteSymbol) throw (Exception)
{
    Q_CHECK_PTR(symbol);
    Q_ASSERT(mSymbols.contains(symbol->getUuid()));

    // the symbol cannot be removed if there are already netpoints connected to it!
    /*if (symbol->getNetPointCount() > 0)
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
            .arg(symbol->getUuid().toString()).arg(symbol->getNetPointCount()),
            QString(tr("There are already netpoints connected to the symbol \"%1\"!"))
            .arg(symbol->getUuid().toString()));
    }*/

    QDomElement parent = mXmlFile->getRoot().firstChildElement("symbols");
    symbol->removeFromSchematic(*this, fromDomTree, parent); // can throw an exception

    mSymbols.remove(symbol->getUuid());

    if (deleteSymbol)
        delete symbol;
}

/*****************************************************************************************
 *  SchematicNetPoint Methods
 ****************************************************************************************/

SchematicNetPoint* Schematic::getNetPointByUuid(const QUuid& uuid) const noexcept
{
    return mNetPoints.value(uuid, 0);
}

SchematicNetPoint* Schematic::createNetPoint(const QUuid& netsignal, const Point& position) throw (Exception)
{
    return SchematicNetPoint::create(*this, mXmlFile->getDocument(), netsignal, position);
}

SchematicNetPoint* Schematic::createNetPoint(const QUuid& symbol, const QUuid& pin) throw (Exception)
{
    return SchematicNetPoint::create(*this, mXmlFile->getDocument(), symbol, pin);
}

void Schematic::addNetPoint(SchematicNetPoint* netpoint, bool toDomTree) throw (Exception)
{
    Q_CHECK_PTR(netpoint);

    // check if there is no netpoint with the same uuid in the list
    if (getNetPointByUuid(netpoint->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netpoint->getUuid().toString(),
            QString(tr("There is already a netpoint with the UUID \"%1\"!"))
            .arg(netpoint->getUuid().toString()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netpoints");
    netpoint->addToSchematic(*this, toDomTree, parent); // can throw an exception

    mNetPoints.insert(netpoint->getUuid(), netpoint);
}

void Schematic::removeNetPoint(SchematicNetPoint* netpoint, bool fromDomTree,
                               bool deleteNetPoint) throw (Exception)
{
    Q_CHECK_PTR(netpoint);
    Q_ASSERT(mNetPoints.contains(netpoint->getUuid()));

    // the netpoint cannot be removed if there are already netlines connected to it!
    if (netpoint->getLines().count() > 0)
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
            .arg(netpoint->getUuid().toString()).arg(netpoint->getLines().count()),
            QString(tr("There are already netlines connected to the netpoint \"%1\"!"))
            .arg(netpoint->getUuid().toString()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netpoints");
    netpoint->removeFromSchematic(*this, fromDomTree, parent); // can throw an exception

    mNetPoints.remove(netpoint->getUuid());

    if (deleteNetPoint)
        delete netpoint;
}

/*****************************************************************************************
 *  SchematicNetLine Methods
 ****************************************************************************************/

SchematicNetLine* Schematic::getNetLineByUuid(const QUuid& uuid) const noexcept
{
    return mNetLines.value(uuid, 0);
}

SchematicNetLine* Schematic::createNetLine(const QUuid& startPoint, const QUuid& endPoint) throw (Exception)
{
    return SchematicNetLine::create(*this, mXmlFile->getDocument(), startPoint, endPoint);
}

void Schematic::addNetLine(SchematicNetLine* netline, bool toDomTree) throw (Exception)
{
    Q_CHECK_PTR(netline);

    // check if there is no netline with the same uuid in the list
    if (getNetLineByUuid(netline->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netline->getUuid().toString(),
            QString(tr("There is already a netline with the UUID \"%1\"!"))
            .arg(netline->getUuid().toString()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netlines");
    netline->addToSchematic(*this, toDomTree, parent); // can throw an exception

    mNetLines.insert(netline->getUuid(), netline);
}

void Schematic::removeNetLine(SchematicNetLine* netline, bool fromDomTree,
                              bool deleteNetLine) throw (Exception)
{
    Q_CHECK_PTR(netline);
    Q_ASSERT(mNetLines.contains(netline->getUuid()));

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netlines");
    netline->removeFromSchematic(*this, fromDomTree, parent); // can throw an exception

    mNetLines.remove(netline->getUuid());

    if (deleteNetLine)
        delete netline;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Schematic::removeFiles() const throw (Exception)
{
    mXmlFile->remove();
}

bool Schematic::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // save symbol instances
    foreach (SymbolInstance* symbol, mSymbols)
    {
        if (!symbol->save(toOriginal, errors))
            success = false;
    }

    // save netpoints
    foreach (SchematicNetPoint* point, mNetPoints)
    {
        if (!point->save(toOriginal, errors))
            success = false;
    }

    // save netlines
    foreach (SchematicNetLine* line, mNetLines)
    {
        if (!line->save(toOriginal, errors))
            success = false;
    }

    // save schematic XML file
    try
    {
        mXmlFile->save(toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    return success;
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

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Schematic* Schematic::create(Project& project, const FilePath& filepath,
                             const QString& name) throw (Exception)
{
    // create XML file with root node
    XmlFile* file = XmlFile::create(filepath, "schematic", 0);

    // create node "meta" with schematic UUID and name
    QDomElement metaNode = file->getDocument().createElement("meta");
    QDomElement uuidNode = file->getDocument().createElement("uuid");
    QDomElement nameNode = file->getDocument().createElement("name");
    QDomText uuidText = file->getDocument().createTextNode(QUuid::createUuid().toString());
    QDomText nameText = file->getDocument().createTextNode(name);
    uuidNode.appendChild(uuidText);
    nameNode.appendChild(nameText);
    metaNode.appendChild(uuidNode);
    metaNode.appendChild(nameNode);
    file->getRoot().appendChild(metaNode);

    // create node "symbols" (empty)
    QDomElement symbolsNode = file->getDocument().createElement("symbols");
    file->getRoot().appendChild(symbolsNode);

    try
    {
        file->save(false); // write new (temporary) XML file to filesystem
        Schematic* schematic = new Schematic(project, filepath, true, false, true); // create new Schematic
        delete file; // this will remove the temporary file, so don't do this earlier!
        return schematic;
    }
    catch (Exception& e)
    {
        delete file;
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
