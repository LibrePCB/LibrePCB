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
#include "schematicnetpoint.h"
#include "schematic.h"
#include "schematicnetline.h"
#include "../project.h"
#include "../circuit/circuit.h"
#include "../circuit/netsignal.h"
#include "../../common/schematiclayer.h"
#include "symbolinstance.h"
#include "symbolpininstance.h"
#include "../circuit/gencompsignalinstance.h"
#include "../../library/symbolgraphicsitem.h"
#include "../../workspace/workspace.h"
#include "../../workspace/settings/workspacesettings.h"
#include "../erc/ercmsg.h"
#include "../../common/file_io/xmldomelement.h"

namespace project {

/*****************************************************************************************
 *  Class SchematicNetPointGraphicsItem
 ****************************************************************************************/

QRectF SchematicNetPointGraphicsItem::sBoundingRect;

// Constructors / Destructor
SchematicNetPointGraphicsItem::SchematicNetPointGraphicsItem(Schematic& schematic,
                                                             SchematicNetPoint& point) noexcept :
    QGraphicsItem(), mSchematic(schematic), mPoint(point), mLayer(nullptr)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(Schematic::ZValue_NetPoints);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    mLayer = mSchematic.getProject().getSchematicLayer(SchematicLayer::Nets);
    Q_ASSERT(mLayer);

    if (sBoundingRect.isNull())
    {
        qreal radius = SchematicNetPoint::getCircleRadius().toPx();
        sBoundingRect = QRectF(-radius, -radius, 2*radius, 2*radius);
    }

    updateCacheAndRepaint();
}

SchematicNetPointGraphicsItem::~SchematicNetPointGraphicsItem() noexcept
{
}

void SchematicNetPointGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const bool highlight = option->state & QStyle::State_Selected;

    if (mPointVisible)
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(mLayer->getColor(highlight), Qt::SolidPattern));
        painter->drawEllipse(sBoundingRect);
    }

#ifdef QT_DEBUG
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    if ((!mPointVisible) && (!deviceIsPrinter) && Workspace::instance().getSettings().getDebugTools()->getShowAllSchematicNetpoints())
    {
        // draw circle
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(sBoundingRect);
    }
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect()))
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(sBoundingRect);
    }
#endif
}

void SchematicNetPointGraphicsItem::updateCacheAndRepaint() noexcept
{
    mPointVisible = (((mPoint.getLines().count() > 1) && (mPoint.isAttached())) || (mPoint.getLines().count() > 2));
    update();
}

/*****************************************************************************************
 *  Static Members
 ****************************************************************************************/

const Length SchematicNetPoint::sCircleRadius = Length(600000);

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicNetPoint::SchematicNetPoint(Schematic& schematic, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mCircuit(schematic.getProject().getCircuit()), mSchematic(schematic),
    mGraphicsItem(nullptr), mNetSignal(nullptr), mSymbolInstance(nullptr),
    mPinInstance(nullptr)
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mAttached = domElement.getFirstChild("attached", true)->getText<bool>();
    if (mAttached)
    {
        QUuid symbolInstanceUuid = domElement.getFirstChild("symbol", true)->getText<QUuid>();
        mSymbolInstance = mSchematic.getSymbolByUuid(symbolInstanceUuid);
        if (!mSymbolInstance)
        {
            throw RuntimeError(__FILE__, __LINE__, symbolInstanceUuid.toString(),
                QString(tr("Invalid symbol instance UUID: \"%1\"")).arg(symbolInstanceUuid.toString()));
        }
        QUuid pinUuid = domElement.getFirstChild("pin", true)->getText<QUuid>();
        mPinInstance = mSymbolInstance->getPinInstance(pinUuid);
        if (!mPinInstance)
        {
            throw RuntimeError(__FILE__, __LINE__, pinUuid.toString(),
                QString(tr("Invalid symbol pin instance UUID: \"%1\"")).arg(pinUuid.toString()));
        }
        const GenCompSignalInstance* compSignal = mPinInstance->getGenCompSignalInstance();
        if (!compSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, pinUuid.toString(),
                QString(tr("The symbol pin instance \"%1\" has no signal.")).arg(pinUuid.toString()));
        }
        mNetSignal = compSignal->getNetSignal();
        if (!mNetSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, pinUuid.toString(), QString(tr("The pin of the "
                "netpoint \"%1\" has no netsignal.")).arg(mUuid.toString()));
        }
        mPosition = mPinInstance->getPosition();
    }
    else
    {
        QUuid netSignalUuid = domElement.getFirstChild("netsignal", true)->getText<QUuid>();
        mNetSignal = mSchematic.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
        if(!mNetSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, netSignalUuid.toString(),
                QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid.toString()));
        }

        mPosition.setX(domElement.getFirstChild("position", true)->getAttribute<Length>("x"));
        mPosition.setY(domElement.getFirstChild("position", true)->getAttribute<Length>("y"));
    }

    init();
}

SchematicNetPoint::SchematicNetPoint(Schematic& schematic, NetSignal& netsignal, const Point& position) throw (Exception) :
    QObject(nullptr), mCircuit(schematic.getProject().getCircuit()), mSchematic(schematic),
    mGraphicsItem(nullptr), mNetSignal(nullptr), mSymbolInstance(nullptr),
    mPinInstance(nullptr)
{
    mUuid = QUuid::createUuid(); // generate random UUID
    mAttached = false;
    mNetSignal = &netsignal;
    mPosition = position;
    init();
}

SchematicNetPoint::SchematicNetPoint(Schematic& schematic, SymbolInstance& symbol,
                                     const QUuid& pin) throw (Exception) :
    QObject(nullptr), mCircuit(schematic.getProject().getCircuit()), mSchematic(schematic),
    mGraphicsItem(nullptr), mNetSignal(nullptr), mSymbolInstance(nullptr),
    mPinInstance(nullptr)
{
    mUuid = QUuid::createUuid(); // generate random UUID
    mAttached = true;
    mSymbolInstance = &symbol;
    mPinInstance = mSymbolInstance->getPinInstance(pin);
    if (!mPinInstance)
    {
        throw RuntimeError(__FILE__, __LINE__, pin.toString(),
            QString(tr("Invalid symbol pin instance UUID: \"%1\"")).arg(pin.toString()));
    }
    const GenCompSignalInstance* compSignal = mPinInstance->getGenCompSignalInstance();
    if (!compSignal)
    {
        throw RuntimeError(__FILE__, __LINE__, pin.toString(),
            QString(tr("The symbol pin instance \"%1\" has no signal.")).arg(pin.toString()));
    }
    mNetSignal = compSignal->getNetSignal();
    if (!mNetSignal)
    {
        throw RuntimeError(__FILE__, __LINE__, pin.toString(), QString(tr("The pin of the "
            "netpoint \"%1\" has no netsignal.")).arg(mUuid.toString()));
    }
    mPosition = mPinInstance->getPosition();
    init();
}

void SchematicNetPoint::init() throw (Exception)
{
    // create the graphics item
    mGraphicsItem = new SchematicNetPointGraphicsItem(mSchematic, *this);
    mGraphicsItem->setPos(mPosition.toPxQPointF());

    // create ERC messages
    mErcMsgDeadNetPoint.reset(new ErcMsg(mCircuit.getProject(), *this,
        mUuid.toString(), "Dead", ErcMsg::ErcMsgType_t::SchematicError,
        QString(tr("Dead net point in schematic page \"%1\": %2"))
        .arg(mSchematic.getName()).arg(mUuid.toString())));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SchematicNetPoint::~SchematicNetPoint() noexcept
{
    delete mGraphicsItem;           mGraphicsItem = nullptr;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SchematicNetPoint::setNetSignal(NetSignal& netsignal) throw (Exception)
{
    mNetSignal->unregisterSchematicNetPoint(*this);
    mNetSignal = &netsignal;
    mNetSignal->registerSchematicNetPoint(*this);
}

void SchematicNetPoint::setPosition(const Point& position) noexcept
{
    mPosition = position;
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    updateLines();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicNetPoint::detachFromPin() throw (Exception)
{
    if (!mAttached) throw LogicError(__FILE__, __LINE__);
    mPinInstance->unregisterNetPoint(this);
    mSymbolInstance = nullptr;
    mPinInstance = nullptr;
    mAttached = false;
    mGraphicsItem->updateCacheAndRepaint();
}

void SchematicNetPoint::attachToPin(SymbolInstance* symbol, SymbolPinInstance* pin) throw (Exception)
{
    Q_ASSERT(symbol); Q_ASSERT(pin);
    if (mAttached) throw LogicError(__FILE__, __LINE__);
    const GenCompSignalInstance* compSignal = pin->getGenCompSignalInstance();
    if (!compSignal) throw LogicError(__FILE__, __LINE__);
    const NetSignal* netsignal = compSignal->getNetSignal();
    if (netsignal != mNetSignal) throw LogicError(__FILE__, __LINE__);
    mSymbolInstance = symbol;
    mPinInstance = pin;
    mPinInstance->registerNetPoint(this);
    mPosition = mPinInstance->getPosition();
    mAttached = true;
    mGraphicsItem->updateCacheAndRepaint();
}

void SchematicNetPoint::updateLines() const noexcept
{
    foreach (SchematicNetLine* line, mLines)
        line->updateLine();
}

void SchematicNetPoint::registerNetLine(SchematicNetLine* netline) noexcept
{
    Q_CHECK_PTR(netline);
    Q_ASSERT(!mLines.contains(netline));
    mLines.append(netline);
    netline->updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mLines.isEmpty());
}

void SchematicNetPoint::unregisterNetLine(SchematicNetLine* netline) noexcept
{
    Q_CHECK_PTR(netline);
    Q_ASSERT(mLines.contains(netline));
    mLines.removeOne(netline);
    netline->updateLine();
    mGraphicsItem->updateCacheAndRepaint();
    mErcMsgDeadNetPoint->setVisible(mLines.isEmpty());
}

void SchematicNetPoint::addToSchematic() throw (Exception)
{
    Q_ASSERT(mLines.isEmpty());

    if (mAttached)
    {
        // check if mNetSignal is correct (would be a bug if not)
        if (mNetSignal != mPinInstance->getGenCompSignalInstance()->getNetSignal())
            throw LogicError(__FILE__, __LINE__);
    }

    mNetSignal->registerSchematicNetPoint(*this);
    if (mAttached)
        mPinInstance->registerNetPoint(this);
    mSchematic.addItem(mGraphicsItem);
    mErcMsgDeadNetPoint->setVisible(true);
}

void SchematicNetPoint::removeFromSchematic() throw (Exception)
{
    Q_ASSERT(mLines.isEmpty());

    if (mAttached)
    {
        // check if mNetSignal is correct (would be a bug if not)
        if (mNetSignal != mPinInstance->getGenCompSignalInstance()->getNetSignal())
            throw LogicError(__FILE__, __LINE__);
    }

    mNetSignal->unregisterSchematicNetPoint(*this);
    if (mAttached)
        mPinInstance->unregisterNetPoint(this);
    mSchematic.removeItem(mGraphicsItem);
    mErcMsgDeadNetPoint->setVisible(false);
}

XmlDomElement* SchematicNetPoint::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("netpoint"));
    root->setAttribute("uuid", mUuid);
    root->appendTextChild("attached", mAttached);
    if (mAttached)
    {
        root->appendTextChild("symbol", mSymbolInstance->getUuid());
        root->appendTextChild("pin", mPinInstance->getLibPinUuid());
    }
    else
    {
        root->appendTextChild("netsignal", mNetSignal->getUuid());
        XmlDomElement* position = root->appendChild("position");
        position->setAttribute("x", mPosition.getX());
        position->setAttribute("y", mPosition.getY());
    }
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SchematicNetPoint::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                             return false;
    if (mNetSignal == nullptr)                      return false;
    if (mAttached && (mSymbolInstance == nullptr))  return false;
    if (mAttached && (mPinInstance == nullptr))     return false;
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

uint SchematicNetPoint::extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                                 QList<SchematicNetPoint*>& netpoints,
                                                 bool floatingPoints,
                                                 bool attachedPoints,
                                                 bool floatingPointsFromFloatingLines,
                                                 bool attachedPointsFromFloatingLines,
                                                 bool floatingPointsFromAttachedLines,
                                                 bool attachedPointsFromAttachedLines,
                                                 bool attachedPointsFromSymbols) noexcept
{
    foreach (QGraphicsItem* item, items)
    {
        Q_ASSERT(item); if (!item) continue;
        switch (item->type())
        {
            case CADScene::Type_SchematicNetPoint:
            {
                if (floatingPoints || attachedPoints)
                {
                    SchematicNetPointGraphicsItem* i = qgraphicsitem_cast<SchematicNetPointGraphicsItem*>(item);
                    Q_ASSERT(i); if (!i) break;
                    SchematicNetPoint* p = &i->getNetPoint();
                    if (((!p->isAttached()) && floatingPoints)
                       || (p->isAttached() && attachedPoints))
                    {
                        if (!netpoints.contains(p))
                            netpoints.append(p);
                    }
                }
                break;
            }
            case CADScene::Type_SchematicNetLine:
            {
                if (floatingPointsFromFloatingLines || attachedPointsFromFloatingLines
                 || floatingPointsFromAttachedLines || attachedPointsFromAttachedLines)
                {
                    SchematicNetLineGraphicsItem* i = qgraphicsitem_cast<SchematicNetLineGraphicsItem*>(item);
                    Q_ASSERT(i); if (!i) break;
                    SchematicNetLine* l = &i->getNetLine();
                    SchematicNetPoint* p1 = &i->getNetLine().getStartPoint();
                    SchematicNetPoint* p2 = &i->getNetLine().getEndPoint();
                    if ( ((!l->isAttachedToSymbol()) && (!p1->isAttached()) && floatingPointsFromFloatingLines)
                      || ((!l->isAttachedToSymbol()) && ( p1->isAttached()) && attachedPointsFromFloatingLines)
                      || (( l->isAttachedToSymbol()) && (!p1->isAttached()) && floatingPointsFromAttachedLines)
                      || (( l->isAttachedToSymbol()) && ( p1->isAttached()) && attachedPointsFromAttachedLines))
                    {
                        if (!netpoints.contains(p1))
                            netpoints.append(p1);
                    }
                    if ( ((!l->isAttachedToSymbol()) && (!p2->isAttached()) && floatingPointsFromFloatingLines)
                      || ((!l->isAttachedToSymbol()) && ( p2->isAttached()) && attachedPointsFromFloatingLines)
                      || (( l->isAttachedToSymbol()) && (!p2->isAttached()) && floatingPointsFromAttachedLines)
                      || (( l->isAttachedToSymbol()) && ( p2->isAttached()) && attachedPointsFromAttachedLines))
                    {
                        if (!netpoints.contains(p2))
                            netpoints.append(p2);
                    }
                }
                break;
            }
            case CADScene::Type_Symbol:
            {
                if (attachedPointsFromSymbols)
                {
                    library::SymbolGraphicsItem* i = qgraphicsitem_cast<library::SymbolGraphicsItem*>(item);
                    Q_ASSERT(i); if (!i) break;
                    SymbolInstance* s = i->getSymbolInstance();
                    Q_ASSERT(s); if (!s) break;
                    foreach (const SymbolPinInstance* pin, s->getPinInstances())
                    {
                        SchematicNetPoint* p = pin->getSchematicNetPoint();
                        if ((p) && (!netpoints.contains(p)))
                            netpoints.append(p);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    return netpoints.count();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
