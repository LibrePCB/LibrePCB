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
#include "si_netlabel.h"
#include "../schematic.h"
#include "../../circuit/netsignal.h"
#include "../../circuit/circuit.h"
#include "../../project.h"
#include <eda4ucommon/fileio/xmldomelement.h>
#include <eda4ucommon/graphics/graphicsscene.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SI_NetLabel::SI_NetLabel(Schematic& schematic, const XmlDomElement& domElement) throw (Exception) :
    SI_Base(), mSchematic(schematic), mGraphicsItem(nullptr), mNetSignal(nullptr)
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    QUuid netSignalUuid = domElement.getAttribute<QUuid>("netsignal", true);
    mNetSignal = mSchematic.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if(!mNetSignal)
    {
        throw RuntimeError(__FILE__, __LINE__, netSignalUuid.toString(),
            QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid.toString()));
    }
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mAngle = domElement.getAttribute<Angle>("angle");

    init();
}

SI_NetLabel::SI_NetLabel(Schematic& schematic, NetSignal& netsignal, const Point& position) throw (Exception) :
    SI_Base(), mSchematic(schematic), mGraphicsItem(nullptr), mPosition(position),
    mAngle(0), mNetSignal(&netsignal)
{
    mUuid = QUuid::createUuid(); // generate random UUID
    init();
}

void SI_NetLabel::init() throw (Exception)
{
    // create the graphics item
    mGraphicsItem = new SGI_NetLabel(*this);
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(mAngle.toDeg());

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SI_NetLabel::~SI_NetLabel() noexcept
{
    delete mGraphicsItem;           mGraphicsItem = nullptr;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SI_NetLabel::setNetSignal(NetSignal& netsignal) noexcept
{
    if (&netsignal == mNetSignal) return;
    mNetSignal->unregisterSchematicNetLabel(*this);
    mNetSignal = &netsignal;
    mNetSignal->registerSchematicNetLabel(*this);
    mGraphicsItem->updateCacheAndRepaint();
}

void SI_NetLabel::setPosition(const Point& position) noexcept
{
    if (position == mPosition) return;
    mPosition = position;
    mGraphicsItem->setPos(mPosition.toPxQPointF());
}

void SI_NetLabel::setAngle(const Angle& angle) noexcept
{
    if (angle == mAngle) return;
    mAngle = angle;
    mGraphicsItem->setRotation(mAngle.toDeg());
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_NetLabel::updateText() noexcept
{
    mGraphicsItem->updateCacheAndRepaint();
}

void SI_NetLabel::addToSchematic(GraphicsScene& scene) throw (Exception)
{
    mNetSignal->registerSchematicNetLabel(*this);
    scene.addItem(*mGraphicsItem);
}

void SI_NetLabel::removeFromSchematic(GraphicsScene& scene) throw (Exception)
{
    mNetSignal->unregisterSchematicNetLabel(*this);
    scene.removeItem(*mGraphicsItem);
}

XmlDomElement* SI_NetLabel::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("netlabel"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("x", mPosition.getX());
    root->setAttribute("y", mPosition.getY());
    root->setAttribute("angle", mAngle);
    root->setAttribute("netsignal", mNetSignal->getUuid());
    return root.take();
}

/*****************************************************************************************
 *  Inherited from SI_Base
 ****************************************************************************************/

QPainterPath SI_NetLabel::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void SI_NetLabel::setSelected(bool selected) noexcept
{
    SI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SI_NetLabel::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                             return false;
    if (mNetSignal == nullptr)                      return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
