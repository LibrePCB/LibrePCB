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
#include "../../common/file_io/xmldomelement.h"

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
        throw LogicError(__FILE__, __LINE__, QString(), tr("No Nets Layer found!"));

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

SchematicNetLine::SchematicNetLine(Schematic& schematic, const XmlDomElement& domElement)
                                   throw (Exception) :
    QObject(nullptr), mSchematic(schematic), mGraphicsItem(nullptr),
    mStartPoint(nullptr), mEndPoint(nullptr)
{
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mWidth = domElement.getAttribute<Length>("width");

    QUuid spUuid = domElement.getAttribute<QUuid>("start_point");
    mStartPoint = mSchematic.getNetPointByUuid(spUuid);
    if(!mStartPoint)
    {
        throw RuntimeError(__FILE__, __LINE__, spUuid.toString(),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(spUuid.toString()));
    }

    QUuid epUuid = domElement.getAttribute<QUuid>("end_point");
    mEndPoint = mSchematic.getNetPointByUuid(epUuid);
    if(!mEndPoint)
    {
        throw RuntimeError(__FILE__, __LINE__, epUuid.toString(),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(epUuid.toString()));
    }

    init();
}

SchematicNetLine::SchematicNetLine(Schematic& schematic, SchematicNetPoint& startPoint,
                                   SchematicNetPoint& endPoint, const Length& width) throw (Exception) :
    QObject(nullptr), mSchematic(schematic), mGraphicsItem(nullptr),
    mStartPoint(&startPoint), mEndPoint(&endPoint), mWidth(width)
{
    mUuid = QUuid::createUuid().toString(); // generate random UUID
    init();
}

void SchematicNetLine::init() throw (Exception)
{
    if(mWidth < 0)
    {
        throw RuntimeError(__FILE__, __LINE__, mWidth.toMmString(),
            QString(tr("Invalid net line width: \"%1\"")).arg(mWidth.toMmString()));
    }

    // check if both netpoints have the same netsignal
    if (mStartPoint->getNetSignal() != mEndPoint->getNetSignal())
    {
        throw LogicError(__FILE__, __LINE__, QString(),
            tr("SchematicNetLine: endpoints netsignal mismatch"));
    }

    mGraphicsItem = new SchematicNetLineGraphicsItem(mSchematic, *this);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
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

void SchematicNetLine::addToSchematic() throw (Exception)
{
    mSchematic.addItem(mGraphicsItem);
    mStartPoint->registerNetLine(this);
    mEndPoint->registerNetLine(this);
}

void SchematicNetLine::removeFromSchematic() throw (Exception)
{
    mStartPoint->unregisterNetLine(this);
    mEndPoint->unregisterNetLine(this);
    mSchematic.removeItem(mGraphicsItem);
}

XmlDomElement* SchematicNetLine::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("netline"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("start_point", mStartPoint->getUuid());
    root->setAttribute("end_point", mEndPoint->getUuid());
    root->setAttribute("width", mWidth);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SchematicNetLine::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mStartPoint == nullptr) return false;
    if (mEndPoint == nullptr)   return false;
    if (mWidth < 0)             return false;
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
