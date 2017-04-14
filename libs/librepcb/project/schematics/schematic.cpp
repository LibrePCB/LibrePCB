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
#include "schematic.h"
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/xmldomdocument.h>
#include <librepcb/common/scopeguardlist.h>
#include "../project.h"
#include <librepcb/library/sym/symbolpin.h>
#include "items/si_symbol.h"
#include "items/si_symbolpin.h"
#include "items/si_netpoint.h"
#include "items/si_netline.h"
#include "items/si_netlabel.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Schematic::Schematic(Project& project, const FilePath& filepath, bool restore,
                     bool readOnly, bool create, const QString& newName) throw (Exception):
    QObject(&project), IF_AttributeProvider(), mProject(project), mFilePath(filepath),
    mIsAddedToProject(false)
{
    try
    {
        mGraphicsScene.reset(new GraphicsScene());

        // try to open/create the XML schematic file
        if (create)
        {
            mXmlFile.reset(SmartXmlFile::create(mFilePath));

            // set attributes
            mUuid = Uuid::createRandom();
            mName = newName;

            // load default grid properties
            mGridProperties.reset(new GridProperties());
        }
        else
        {
            mXmlFile.reset(new SmartXmlFile(mFilePath, restore, readOnly));
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
            XmlDomElement& root = doc->getRoot();

            // the schematic seems to be ready to open, so we will create all needed objects

            mUuid = root.getFirstChild("meta/uuid", true, true)->getText<Uuid>(true);
            mName = root.getFirstChild("meta/name", true, true)->getText<QString>(true);

            // Load grid properties
            mGridProperties.reset(new GridProperties(*root.getFirstChild("properties/grid_properties", true, true)));

            // Load all symbols
            for (XmlDomElement* node = root.getFirstChild("symbols/symbol", true, false);
                 node; node = node->getNextSibling("symbol"))
            {
                SI_Symbol* symbol = new SI_Symbol(*this, *node);
                if (getSymbolByUuid(symbol->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__, symbol->getUuid().toStr(),
                        QString(tr("There is already a symbol with the UUID \"%1\"!"))
                        .arg(symbol->getUuid().toStr()));
                }
                mSymbols.append(symbol);
            }

            // Load all netpoints
            for (XmlDomElement* node = root.getFirstChild("netpoints/netpoint", true, false);
                 node; node = node->getNextSibling("netpoint"))
            {
                SI_NetPoint* netpoint = new SI_NetPoint(*this, *node);
                if (getNetPointByUuid(netpoint->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__, netpoint->getUuid().toStr(),
                        QString(tr("There is already a netpoint with the UUID \"%1\"!"))
                        .arg(netpoint->getUuid().toStr()));
                }
                mNetPoints.append(netpoint);
            }

            // Load all netlines
            for (XmlDomElement* node = root.getFirstChild("netlines/netline", true, false);
                 node; node = node->getNextSibling("netline"))
            {
                SI_NetLine* netline = new SI_NetLine(*this, *node);
                if (getNetLineByUuid(netline->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__, netline->getUuid().toStr(),
                        QString(tr("There is already a netline with the UUID \"%1\"!"))
                        .arg(netline->getUuid().toStr()));
                }
                mNetLines.append(netline);
            }

            // Load all netlabels
            for (XmlDomElement* node = root.getFirstChild("netlabels/netlabel", true, false);
                 node; node = node->getNextSibling("netlabel"))
            {
                SI_NetLabel* netlabel = new SI_NetLabel(*this, *node);
                if (getNetLabelByUuid(netlabel->getUuid())) {
                    throw RuntimeError(__FILE__, __LINE__, netlabel->getUuid().toStr(),
                        QString(tr("There is already a netlabel with the UUID \"%1\"!"))
                        .arg(netlabel->getUuid().toStr()));
                }
                mNetLabels.append(netlabel);
            }
        }

        // emit the "attributesChanged" signal when the project has emited it
        connect(&mProject, &Project::attributesChanged, this, &Schematic::attributesChanged);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        qDeleteAll(mNetLabels);         mNetLabels.clear();
        qDeleteAll(mNetLines);          mNetLines.clear();
        qDeleteAll(mNetPoints);         mNetPoints.clear();
        qDeleteAll(mSymbols);           mSymbols.clear();
        mGridProperties.reset();
        mXmlFile.reset();
        mGraphicsScene.reset();
        throw; // ...and rethrow the exception
    }
}

Schematic::~Schematic() noexcept
{
    Q_ASSERT(!mIsAddedToProject);

    // delete all items
    qDeleteAll(mNetLabels);         mNetLabels.clear();
    qDeleteAll(mNetLines);          mNetLines.clear();
    qDeleteAll(mNetPoints);         mNetPoints.clear();
    qDeleteAll(mSymbols);           mSymbols.clear();

    mGridProperties.reset();
    mXmlFile.reset();
    mGraphicsScene.reset();
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

bool Schematic::isEmpty() const noexcept
{
    return (mSymbols.isEmpty() &&
            mNetPoints.isEmpty() &&
            mNetLines.isEmpty() &&
            mNetLabels.isEmpty());
}

QList<SI_Base*> Schematic::getSelectedItems(bool symbolPins,
                                            bool floatingPoints,
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
    // TODO: this method is incredible ugly ;)

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
            if (pin->isSelected() && symbolPins)
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
            if (((!netpoint->isAttachedToPin()) && floatingPoints)
               || (netpoint->isAttachedToPin() && attachedPoints))
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
            if ( ((!netline->isAttachedToSymbol()) && (!p1->isAttachedToPin()) && floatingPointsFromFloatingLines)
              || ((!netline->isAttachedToSymbol()) && ( p1->isAttachedToPin()) && attachedPointsFromFloatingLines)
              || (( netline->isAttachedToSymbol()) && (!p1->isAttachedToPin()) && floatingPointsFromAttachedLines)
              || (( netline->isAttachedToSymbol()) && ( p1->isAttachedToPin()) && attachedPointsFromAttachedLines))
            {
                if (!list.contains(p1))
                    list.append(p1);
            }
            if ( ((!netline->isAttachedToSymbol()) && (!p2->isAttachedToPin()) && floatingPointsFromFloatingLines)
              || ((!netline->isAttachedToSymbol()) && ( p2->isAttachedToPin()) && attachedPointsFromFloatingLines)
              || (( netline->isAttachedToSymbol()) && (!p2->isAttachedToPin()) && floatingPointsFromAttachedLines)
              || (( netline->isAttachedToSymbol()) && ( p2->isAttachedToPin()) && attachedPointsFromAttachedLines))
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
        if (!netpoint->isVisibleJunction()) continue;
        if (netpoint->getGrabAreaScenePx().contains(scenePosPx))
            list.append(netpoint);
    }
    // hidden netpoints
    foreach (SI_NetPoint* netpoint, mNetPoints)
    {
        if (netpoint->isVisibleJunction()) continue;
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

QList<SI_Base*> Schematic::getAllItems() const noexcept
{
    QList<SI_Base*> items;
    foreach (SI_Symbol* symbol, mSymbols)
        items.append(symbol);
    foreach (SI_NetPoint* netpoint, mNetPoints)
        items.append(netpoint);
    foreach (SI_NetLine* netline, mNetLines)
        items.append(netline);
    foreach (SI_NetLabel* netlabel, mNetLabels)
        items.append(netlabel);
    return items;
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

SI_Symbol* Schematic::getSymbolByUuid(const Uuid& uuid) const noexcept
{
    foreach (SI_Symbol* symbol, mSymbols) {
        if (symbol->getUuid() == uuid)
            return symbol;
    }
    return nullptr;
}

void Schematic::addSymbol(SI_Symbol& symbol) throw (Exception)
{
    if ((!mIsAddedToProject) || (mSymbols.contains(&symbol))
        || (&symbol.getSchematic() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no symbol with the same uuid in the list
    if (getSymbolByUuid(symbol.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__, symbol.getUuid().toStr(),
            QString(tr("There is already a symbol with the UUID \"%1\"!"))
            .arg(symbol.getUuid().toStr()));
    }
    // add to schematic
    symbol.addToSchematic(*mGraphicsScene); // can throw
    mSymbols.append(&symbol);
}

void Schematic::removeSymbol(SI_Symbol& symbol) throw (Exception)
{
    if ((!mIsAddedToProject) || (!mSymbols.contains(&symbol))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from schematic
    symbol.removeFromSchematic(*mGraphicsScene); // can throw
    mSymbols.removeOne(&symbol);
}

/*****************************************************************************************
 *  NetPoint Methods
 ****************************************************************************************/

SI_NetPoint* Schematic::getNetPointByUuid(const Uuid& uuid) const noexcept
{
    foreach (SI_NetPoint* netpoint, mNetPoints) {
        if (netpoint->getUuid() == uuid)
            return netpoint;
    }
    return nullptr;
}

void Schematic::addNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    if ((!mIsAddedToProject) || (mNetPoints.contains(&netpoint))
        || (&netpoint.getSchematic() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netpoint with the same uuid in the list
    if (getNetPointByUuid(netpoint.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__, netpoint.getUuid().toStr(),
            QString(tr("There is already a netpoint with the UUID \"%1\"!"))
            .arg(netpoint.getUuid().toStr()));
    }
    // add to schematic
    netpoint.addToSchematic(*mGraphicsScene); // can throw
    mNetPoints.append(&netpoint);
}

void Schematic::removeNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    if ((!mIsAddedToProject) || (!mNetPoints.contains(&netpoint))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from schematic
    netpoint.removeFromSchematic(*mGraphicsScene); // can throw an exception
    mNetPoints.removeOne(&netpoint);
}

/*****************************************************************************************
 *  NetLine Methods
 ****************************************************************************************/

SI_NetLine* Schematic::getNetLineByUuid(const Uuid& uuid) const noexcept
{
    foreach (SI_NetLine* netline, mNetLines) {
        if (netline->getUuid() == uuid)
            return netline;
    }
    return nullptr;
}

void Schematic::addNetLine(SI_NetLine& netline) throw (Exception)
{
    if ((!mIsAddedToProject) || (mNetLines.contains(&netline))
        || (&netline.getSchematic() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netline with the same uuid in the list
    if (getNetLineByUuid(netline.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__, netline.getUuid().toStr(),
            QString(tr("There is already a netline with the UUID \"%1\"!"))
            .arg(netline.getUuid().toStr()));
    }
    // add to schematic
    netline.addToSchematic(*mGraphicsScene); // can throw
    mNetLines.append(&netline);
}

void Schematic::removeNetLine(SI_NetLine& netline) throw (Exception)
{
    if ((!mIsAddedToProject) || (!mNetLines.contains(&netline))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from schematic
    netline.removeFromSchematic(*mGraphicsScene); // can throw
    mNetLines.removeOne(&netline);
}

/*****************************************************************************************
 *  NetLabel Methods
 ****************************************************************************************/

SI_NetLabel* Schematic::getNetLabelByUuid(const Uuid& uuid) const noexcept
{
    foreach (SI_NetLabel* netlabel, mNetLabels) {
        if (netlabel->getUuid() == uuid)
            return netlabel;
    }
    return nullptr;
}

void Schematic::addNetLabel(SI_NetLabel& netlabel) throw (Exception)
{
    if ((!mIsAddedToProject) || (mNetLabels.contains(&netlabel))
        || (&netlabel.getSchematic() != this))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netlabel with the same uuid in the list
    if (getNetLabelByUuid(netlabel.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__, netlabel.getUuid().toStr(),
            QString(tr("There is already a netlabel with the UUID \"%1\"!"))
            .arg(netlabel.getUuid().toStr()));
    }
    // add to schematic
    netlabel.addToSchematic(*mGraphicsScene); // can throw
    mNetLabels.append(&netlabel);
}

void Schematic::removeNetLabel(SI_NetLabel& netlabel) throw (Exception)
{
    if ((!mIsAddedToProject) || (!mNetLabels.contains(&netlabel))) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from schematic
    netlabel.removeFromSchematic(*mGraphicsScene); // can throw
    mNetLabels.removeOne(&netlabel);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Schematic::addToProject() throw (Exception)
{
    if (mIsAddedToProject) {
        throw LogicError(__FILE__, __LINE__);
    }
    QList<SI_Base*> items = getAllItems();
    ScopeGuardList sgl(items.count());
    for (int i = 0; i < items.count(); ++i) {
        SI_Base* item = items.at(i);
        item->addToSchematic(*mGraphicsScene); // can throw
        sgl.add([this, item](){item->removeFromSchematic(*mGraphicsScene);});
    }
    mIsAddedToProject = true;
    updateIcon();
    sgl.dismiss();
}

void Schematic::removeFromProject() throw (Exception)
{
    if (!mIsAddedToProject) {
        throw LogicError(__FILE__, __LINE__);
    }
    QList<SI_Base*> items = getAllItems();
    ScopeGuardList sgl(items.count());
    for (int i = items.count()-1; i >= 0; --i) {
        SI_Base* item = items.at(i);
        item->removeFromSchematic(*mGraphicsScene); // can throw
        sgl.add([this, item](){item->addToSchematic(*mGraphicsScene);});
    }
    mIsAddedToProject = false;
    sgl.dismiss();
}

bool Schematic::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // save schematic XML file
    try
    {
        if (mIsAddedToProject)
        {
            XmlDomDocument doc(*serializeToXmlDomElement("schematic"));
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
    view.setScene(mGraphicsScene.data());
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
    mGraphicsScene->render(&painter, QRectF(), mGraphicsScene->itemsBoundingRect(), Qt::KeepAspectRatio);
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
            return value = QString::number(mProject.getSchematicIndex(*this) + 1), true;
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

void Schematic::serialize(XmlDomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    XmlDomElement* meta = root.appendChild("meta");
    meta->appendTextChild("uuid", mUuid);
    meta->appendTextChild("name", mName);
    XmlDomElement* properties = root.appendChild("properties");
    properties->appendChild(mGridProperties->serializeToXmlDomElement("grid_properties"));
    root.appendChild(serializePointerContainer(mSymbols, "symbols", "symbol"));
    root.appendChild(serializePointerContainer(mNetPoints, "netpoints", "netpoint"));
    root.appendChild(serializePointerContainer(mNetLines, "netlines", "netline"));
    root.appendChild(serializePointerContainer(mNetLabels, "netlabels", "netlabel"));
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
} // namespace librepcb
