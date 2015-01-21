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
#include <QPrinter>
#include <QGraphicsLineItem>
#include "schematicnetline.h"
#include "schematic.h"
#include "schematicnetpoint.h"
#include "../project.h"
#include "../../common/schematiclayer.h"
#include "../circuit/netsignal.h"
#include "../../library/symbolgraphicsitem.h"
#include "symbolinstance.h"
#include "symbolpininstance.h"
#include "../../workspace/workspace.h"
#include "../../workspace/settings/workspacesettings.h"

namespace project {

/*****************************************************************************************
 *  Class SchematicNetLineGraphicsItem
 ****************************************************************************************/

// Constructors / Destructor
SchematicNetLineGraphicsItem::SchematicNetLineGraphicsItem(Schematic& schematic,
                                                           SchematicNetLine& line) throw (Exception) :
    QGraphicsLineItem(), mSchematic(schematic), mLine(line), mLayer(0)
{
    mLayer = mSchematic.getProject().getSchematicLayer(SchematicLayer::Nets);
    if (!mLayer)
    {
        throw LogicError(__FILE__, __LINE__, QString(),
            QCoreApplication::translate("SchematicNetLineGraphicsItem",
                                        "No Nets Layer found!"));
    }

    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(Schematic::ZValue_NetLines);
}

SchematicNetLineGraphicsItem::~SchematicNetLineGraphicsItem() noexcept
{
}

QPainterPath SchematicNetLineGraphicsItem::shape() const
{
    QPainterPath path;
    path.moveTo(line().p1());
    path.lineTo(line().p2());
    QPainterPathStroker ps;
    ps.setCapStyle(Qt::RoundCap);
    Length width = (mLine.getWidth() > Length(1270000) ? mLine.getWidth() : Length(1270000));
    ps.setWidth(width.toPx());
    return ps.createStroke(path);
}

void SchematicNetLineGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    bool highlight = option->state & QStyle::State_Selected;

    // draw line
    painter->setPen(QPen(mLayer->getColor(highlight), mLine.getWidth().toPx(),
                         Qt::SolidLine, Qt::RoundCap));
    painter->drawLine(line());

#ifdef QT_DEBUG
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowSchematicNetlinesNetsignals()))
    {
        // draw net signal name
        painter->setPen(QPen(mLayer->getColor(highlight), 0));
        QFont font;
        font.setFamily("Monospace");
        font.setPixelSize(3);
        font.setStyleHint(QFont::TypeWriter);
        font.setStyleStrategy(QFont::ForceOutline);
        painter->setFont(font);
        painter->drawText(line().pointAt((qreal)0.5), mLine.getNetSignal()->getName());
    }
#endif
}

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicNetLine::SchematicNetLine(Schematic& schematic, const QDomElement& domElement)
                                   throw (Exception) :
    QObject(0), mSchematic(schematic), mDomElement(domElement), mGraphicsItem(0),
    mStartPoint(0), mEndPoint(0)
{
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid net line UUID: \"%1\""))
            .arg(mDomElement.attribute("uuid")));
    }

    QString width = mDomElement.attribute(QStringLiteral("width"));
    mWidth.setLengthMm(width);
    if(mWidth < 0)
    {
        throw RuntimeError(__FILE__, __LINE__, width,
            QString(tr("Invalid net line width: \"%1\"")).arg(width));
    }

    mStartPoint = mSchematic.getNetPointByUuid(mDomElement.attribute("start_point"));
    if(!mStartPoint)
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("start_point"),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(mDomElement.attribute("start_point")));
    }

    mEndPoint = mSchematic.getNetPointByUuid(mDomElement.attribute("end_point"));
    if(!mEndPoint)
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("end_point"),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(mDomElement.attribute("end_point")));
    }

    // check if both netpoints have the same netsignal
    if (mStartPoint->getNetSignal() != mEndPoint->getNetSignal())
    {
        throw LogicError(__FILE__, __LINE__, QString(),
            tr("SchematicNetLine: endpoints netsignal mismatch"));
    }

    mGraphicsItem = new SchematicNetLineGraphicsItem(mSchematic, *this);
}

SchematicNetLine::~SchematicNetLine() noexcept
{
    delete mGraphicsItem;       mGraphicsItem = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

NetSignal* SchematicNetLine::getNetSignal() const noexcept
{
    Q_ASSERT(mStartPoint->getNetSignal() == mEndPoint->getNetSignal());
    return mStartPoint->getNetSignal();
}

bool SchematicNetLine::isAttachedToSymbol() const noexcept
{
    return (mStartPoint->isAttached() || mEndPoint->isAttached());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SchematicNetLine::setWidth(const Length& width) noexcept
{
    Q_ASSERT(width >= 0);
    mWidth = width;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicNetLine::updateLine() noexcept
{
    QLineF line(mStartPoint->getPosition().toPxQPointF(), mEndPoint->getPosition().toPxQPointF());
    mGraphicsItem->setLine(line);
}

void SchematicNetLine::addToSchematic(Schematic& schematic, bool addNode,
                                      QDomElement& parent) throw (Exception)
{
    if (addNode)
    {
        if (parent.nodeName() != "netlines")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.appendChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not append DOM node!"));
    }

    schematic.addItem(mGraphicsItem);
    mStartPoint->registerNetLine(this);
    mEndPoint->registerNetLine(this);
}

void SchematicNetLine::removeFromSchematic(Schematic& schematic, bool removeNode,
                                           QDomElement& parent) throw (Exception)
{
    if (removeNode)
    {
        if (parent.nodeName() != "netlines")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.removeChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not remove node from DOM tree!"));
    }

    mStartPoint->unregisterNetLine(this);
    mEndPoint->unregisterNetLine(this);
    schematic.removeItem(mGraphicsItem);
}

bool SchematicNetLine::save(bool toOriginal, QStringList& errors) noexcept
{
    Q_UNUSED(toOriginal);
    Q_UNUSED(errors);
    mDomElement.setAttribute(QStringLiteral("width"), mWidth.toMmString());
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

uint SchematicNetLine::extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                                QList<SchematicNetLine*>& netlines,
                                                bool floatingLines,
                                                bool attachedLines,
                                                bool attachedLinesFromSymbols) noexcept
{
    foreach (QGraphicsItem* item, items)
    {
        Q_ASSERT(item); if (!item) continue;
        switch (item->type())
        {
            case CADScene::Type_SchematicNetLine:
            {
                SchematicNetLineGraphicsItem* i = qgraphicsitem_cast<SchematicNetLineGraphicsItem*>(item);
                Q_ASSERT(i); if (!i) break;
                SchematicNetLine* l = &i->getNetLine();
                if (((!l->isAttachedToSymbol()) && floatingLines)
                   || (l->isAttachedToSymbol() && attachedLines))
                {
                    if (!netlines.contains(l))
                        netlines.append(l);
                }
                break;
            }
            case CADScene::Type_Symbol:
            {
                if (attachedLinesFromSymbols)
                {
                    library::SymbolGraphicsItem* i = qgraphicsitem_cast<library::SymbolGraphicsItem*>(item);
                    Q_ASSERT(i); if (!i) break;
                    SymbolInstance* s = i->getSymbolInstance();
                    Q_ASSERT(s); if (!s) break;
                    foreach (const SymbolPinInstance* pin, s->getPinInstances())
                    {
                        SchematicNetPoint* p = pin->getSchematicNetPoint();
                        if (p)
                        {
                            foreach (SchematicNetLine* l, p->getLines())
                            {
                                if (!netlines.contains(l))
                                    netlines.append(l);
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    return netlines.count();
}

SchematicNetLine* SchematicNetLine::create(Schematic& schematic, QDomDocument& doc,
                                           const QUuid& startPoint, const QUuid& endPoint,
                                           const Length& width) throw (Exception)
{
    QDomElement node = doc.createElement("netline");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    node.setAttribute("start_point", startPoint.toString());
    node.setAttribute("end_point", endPoint.toString());
    node.setAttribute("width", width.toMmString());

    // create and return the new SchematicNetLine object
    return new SchematicNetLine(schematic, node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
