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
#include "../../workspace/workspace.h"
#include "../../workspace/settings/workspacesettings.h"

namespace project {

/*****************************************************************************************
 *  Class SchematicNetPointGraphicsItem
 ****************************************************************************************/

// Constructors / Destructor
SchematicNetPointGraphicsItem::SchematicNetPointGraphicsItem(Schematic& schematic,
                                                             SchematicNetPoint& point) throw (Exception) :
    QGraphicsItem(), mSchematic(schematic), mPoint(point), mLayer(0)
{
    mLayer = mSchematic.getProject().getSchematicLayer(SchematicLayer::Nets);
    if (!mLayer)
    {
        throw LogicError(__FILE__, __LINE__, QString(),
            QCoreApplication::translate("SchematicNetPointGraphicsItem",
                                        "No Nets Layer found!"));
    }

    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(Schematic::ZValue_NetPoints);
}

SchematicNetPointGraphicsItem::~SchematicNetPointGraphicsItem() noexcept
{
}

QRectF SchematicNetPointGraphicsItem::boundingRect() const
{
    qreal radius = SchematicNetPoint::getCircleRadius().toPx() * 1.5f;
    return QRectF(-radius, -radius, 2*radius, 2*radius);
}

void SchematicNetPointGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    qreal radius = SchematicNetPoint::getCircleRadius().toPx();
    bool highlight = option->state & QStyle::State_Selected;
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);

    if ((  (mPoint.getLines().count() > 1) && (mPoint.isAttached()))
        || (mPoint.getLines().count() > 2))
    {
        painter->setPen(QPen(mLayer->getColor(highlight), 0));
        painter->setBrush(QBrush(mLayer->getColor(highlight), Qt::SolidPattern));
        painter->drawEllipse(QPointF(0, 0), radius, radius);
    }
    else if (!deviceIsPrinter)
    {
#ifdef QT_DEBUG
        if (Workspace::instance().getSettings().getDebugTools()->getShowAllSchematicNetpoints())
        {
            painter->setPen(QPen(Qt::red, 0));
            painter->drawEllipse(QPointF(0, 0), radius, radius);
        }
#endif
    }
}

/*****************************************************************************************
 *  Static Members
 ****************************************************************************************/

const Length SchematicNetPoint::sCircleRadius = Length(600000);

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicNetPoint::SchematicNetPoint(Schematic& schematic, const QDomElement& domElement)
                                     throw (Exception) :
    QObject(0), mSchematic(schematic), mDomElement(domElement), mGraphicsItem(nullptr),
    mNetSignal(nullptr), mSymbolInstance(nullptr), mPinInstance(nullptr)
{
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(mDomElement.attribute("uuid")));
    }

    mAttached = (mDomElement.firstChildElement("attached").text() == "true");

    if (mAttached)
    {
        QString symbolInstanceUuid = mDomElement.firstChildElement("symbol").text();
        mSymbolInstance = mSchematic.getSymbolByUuid(symbolInstanceUuid);
        if (!mSymbolInstance)
        {
            throw RuntimeError(__FILE__, __LINE__, symbolInstanceUuid,
                QString(tr("Invalid symbol instance UUID: \"%1\"")).arg(symbolInstanceUuid));
        }
        QString pinUuid = mDomElement.firstChildElement("pin").text();
        mPinInstance = mSymbolInstance->getPinInstance(pinUuid);
        if (!mPinInstance)
        {
            throw RuntimeError(__FILE__, __LINE__, pinUuid,
                QString(tr("Invalid symbol pin instance UUID: \"%1\"")).arg(pinUuid));
        }
        const GenCompSignalInstance* compSignal = mPinInstance->getGenCompSignalInstance();
        if (!compSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, pinUuid,
                QString(tr("The symbol pin instance \"%1\" has no signal.")).arg(pinUuid));
        }
        mNetSignal = compSignal->getNetSignal();
        if (!mNetSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, pinUuid, QString(tr("The pin of the "
                "netpoint \"%1\" has no netsignal.")).arg(mUuid.toString()));
        }
        mPosition = mPinInstance->getPosition();
    }
    else
    {
        QString netSignalUuid = mDomElement.firstChildElement("netsignal").text();
        mNetSignal = mSchematic.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
        if(!mNetSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, netSignalUuid,
                QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid));
        }

        mPosition.setX(Length::fromMm(mDomElement.firstChildElement("position").attribute("x")));
        mPosition.setY(Length::fromMm(mDomElement.firstChildElement("position").attribute("y")));
    }

    // create the graphics item
    mGraphicsItem = new SchematicNetPointGraphicsItem(mSchematic, *this);
    mGraphicsItem->setPos(mPosition.toPxQPointF());
}

SchematicNetPoint::~SchematicNetPoint() noexcept
{
    delete mGraphicsItem;           mGraphicsItem = 0;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SchematicNetPoint::setPosition(const Point& position) noexcept
{
    mPosition = position;
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    updateLines();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

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
}

void SchematicNetPoint::unregisterNetLine(SchematicNetLine* netline) noexcept
{
    Q_CHECK_PTR(netline);
    Q_ASSERT(mLines.contains(netline));
    mLines.removeOne(netline);
    netline->updateLine();
}

void SchematicNetPoint::addToSchematic(Schematic& schematic, bool addNode,
                                       QDomElement& parent) throw (Exception)
{
    if (mAttached)
    {
        // check if mNetSignal is correct (would be a bug if not)
        if (mNetSignal != mPinInstance->getGenCompSignalInstance()->getNetSignal())
            throw LogicError(__FILE__, __LINE__);
    }

    if (addNode)
    {
        if (parent.nodeName() != "netpoints")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.appendChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not append DOM node!"));
    }

    mNetSignal->registerSchematicNetPoint(this);
    if (mAttached)
        mPinInstance->registerNetPoint(this);
    schematic.addItem(mGraphicsItem);
}

void SchematicNetPoint::removeFromSchematic(Schematic& schematic, bool removeNode,
                                            QDomElement& parent) throw (Exception)
{
    if (mAttached)
    {
        // check if mNetSignal is correct (would be a bug if not)
        if (mNetSignal != mPinInstance->getGenCompSignalInstance()->getNetSignal())
            throw LogicError(__FILE__, __LINE__);
    }

    if (removeNode)
    {
        if (parent.nodeName() != "netpoints")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.removeChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not remove node from DOM tree!"));
    }

    mNetSignal->unregisterSchematicNetPoint(this);
    if (mAttached)
        mPinInstance->unregisterNetPoint(this);
    schematic.removeItem(mGraphicsItem);
}

bool SchematicNetPoint::save(bool toOriginal, QStringList& errors) noexcept
{
    Q_UNUSED(toOriginal); Q_UNUSED(errors);
    mDomElement.removeChild(mDomElement.firstChildElement("attached"));
    mDomElement.removeChild(mDomElement.firstChildElement("netsignal"));
    mDomElement.removeChild(mDomElement.firstChildElement("position"));
    mDomElement.removeChild(mDomElement.firstChildElement("symbol"));
    mDomElement.removeChild(mDomElement.firstChildElement("pin"));
    if (mAttached)
    {
        QDomElement attachedNode = mDomElement.ownerDocument().createElement("attached");
        QDomText attachedText = mDomElement.ownerDocument().createTextNode("true");
        attachedNode.appendChild(attachedText);
        mDomElement.appendChild(attachedNode);
        QDomElement symbolNode = mDomElement.ownerDocument().createElement("symbol");
        QDomText symbolText = mDomElement.ownerDocument().createTextNode(mSymbolInstance->getUuid().toString());
        symbolNode.appendChild(symbolText);
        mDomElement.appendChild(symbolNode);
        QDomElement pinNode = mDomElement.ownerDocument().createElement("pin");
        QDomText pinText = mDomElement.ownerDocument().createTextNode(mPinInstance->getLibPinUuid().toString());
        pinNode.appendChild(pinText);
        mDomElement.appendChild(pinNode);
    }
    else
    {
        QDomElement attachedNode = mDomElement.ownerDocument().createElement("attached");
        QDomText attachedText = mDomElement.ownerDocument().createTextNode("false");
        attachedNode.appendChild(attachedText);
        mDomElement.appendChild(attachedNode);
        QDomElement netsignalNode = mDomElement.ownerDocument().createElement("netsignal");
        QDomText netsignalText = mDomElement.ownerDocument().createTextNode(mNetSignal->getUuid().toString());
        netsignalNode.appendChild(netsignalText);
        mDomElement.appendChild(netsignalNode);
        QDomElement posNode = mDomElement.ownerDocument().createElement("position");
        posNode.setAttribute("x", mPosition.getX().toMmString());
        mDomElement.firstChildElement("position").setAttribute("x", mPosition.getX().toMmString());
        posNode.setAttribute("y", mPosition.getY().toMmString());
        mDomElement.appendChild(posNode);
    }
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SchematicNetPoint* SchematicNetPoint::create(Schematic& schematic, QDomDocument& doc,
                                             const QUuid& netsignal, const Point& position) throw (Exception)
{
    QDomElement node = doc.createElement("netpoint");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    QDomElement attachedNode = doc.createElement("attached");
    QDomText attachedText = doc.createTextNode("false");
    attachedNode.appendChild(attachedText);
    node.appendChild(attachedNode);
    QDomElement netsignalNode = doc.createElement("netsignal");
    QDomText netsignalText = doc.createTextNode(netsignal.toString());
    netsignalNode.appendChild(netsignalText);
    node.appendChild(netsignalNode);
    QDomElement posNode = doc.createElement("position");
    posNode.setAttribute("x", position.getX().toMmString());
    posNode.setAttribute("y", position.getY().toMmString());
    node.appendChild(posNode);

    // create and return the new SchematicNetPoint object
    return new SchematicNetPoint(schematic, node);
}

SchematicNetPoint* SchematicNetPoint::create(Schematic& schematic, QDomDocument& doc,
                                             const QUuid& symbol, const QUuid& pin) throw (Exception)
{
    QDomElement node = doc.createElement("netpoint");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    QDomElement attachedNode = doc.createElement("attached");
    QDomText attachedText = doc.createTextNode("true");
    attachedNode.appendChild(attachedText);
    node.appendChild(attachedNode);
    QDomElement symbolNode = doc.createElement("symbol");
    QDomText symbolText = doc.createTextNode(symbol.toString());
    symbolNode.appendChild(symbolText);
    node.appendChild(symbolNode);
    QDomElement pinNode = doc.createElement("pin");
    QDomText pinText = doc.createTextNode(pin.toString());
    pinNode.appendChild(pinText);
    node.appendChild(pinNode);

    // create and return the new SchematicNetPoint object
    return new SchematicNetPoint(schematic, node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
