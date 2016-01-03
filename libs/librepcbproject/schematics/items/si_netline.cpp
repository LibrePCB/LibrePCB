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
#include "si_netline.h"
#include "../schematic.h"
#include "si_netpoint.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include "si_symbol.h"
#include "si_symbolpin.h"
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcbcommon/graphics/graphicsscene.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SI_NetLine::SI_NetLine(Schematic& schematic, const XmlDomElement& domElement) throw (Exception) :
    SI_Base(), mSchematic(schematic), mGraphicsItem(nullptr), mPosition(0, 0),
    mStartPoint(nullptr), mEndPoint(nullptr)
{
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mWidth = domElement.getAttribute<Length>("width", true);

    Uuid spUuid = domElement.getAttribute<Uuid>("start_point", true);
    mStartPoint = mSchematic.getNetPointByUuid(spUuid);
    if(!mStartPoint)
    {
        throw RuntimeError(__FILE__, __LINE__, spUuid.toStr(),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(spUuid.toStr()));
    }

    Uuid epUuid = domElement.getAttribute<Uuid>("end_point", true);
    mEndPoint = mSchematic.getNetPointByUuid(epUuid);
    if(!mEndPoint)
    {
        throw RuntimeError(__FILE__, __LINE__, epUuid.toStr(),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(epUuid.toStr()));
    }

    init();
}

SI_NetLine::SI_NetLine(Schematic& schematic, SI_NetPoint& startPoint,
                                   SI_NetPoint& endPoint, const Length& width) throw (Exception) :
    SI_Base(), mSchematic(schematic), mGraphicsItem(nullptr),
    mStartPoint(&startPoint), mEndPoint(&endPoint), mWidth(width)
{
    mUuid = Uuid::createRandom(); // generate random UUID
    init();
}

void SI_NetLine::init() throw (Exception)
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

    mGraphicsItem = new SGI_NetLine(*this);
    updateLine();

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SI_NetLine::~SI_NetLine() noexcept
{
    delete mGraphicsItem;       mGraphicsItem = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Project& SI_NetLine::getProject() const noexcept
{
    return mSchematic.getProject();
}

NetSignal* SI_NetLine::getNetSignal() const noexcept
{
    Q_ASSERT(mStartPoint->getNetSignal() == mEndPoint->getNetSignal());
    return mStartPoint->getNetSignal();
}

bool SI_NetLine::isAttachedToSymbol() const noexcept
{
    return (mStartPoint->isAttached() || mEndPoint->isAttached());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SI_NetLine::setWidth(const Length& width) noexcept
{
    Q_ASSERT(width >= 0);
    mWidth = width;
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_NetLine::updateLine() noexcept
{
    mPosition = (mStartPoint->getPosition() + mEndPoint->getPosition()) / 2;
    mGraphicsItem->updateCacheAndRepaint();
}

void SI_NetLine::addToSchematic(GraphicsScene& scene) throw (Exception)
{
    mStartPoint->registerNetLine(*this);
    mEndPoint->registerNetLine(*this);
    scene.addItem(*mGraphicsItem);
}

void SI_NetLine::removeFromSchematic(GraphicsScene& scene) throw (Exception)
{
    scene.removeItem(*mGraphicsItem);
    mStartPoint->unregisterNetLine(*this);
    mEndPoint->unregisterNetLine(*this);
}

XmlDomElement* SI_NetLine::serializeToXmlDomElement() const throw (Exception)
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
 *  Inherited from SI_Base
 ****************************************************************************************/

QPainterPath SI_NetLine::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->shape();
}

void SI_NetLine::setSelected(bool selected) noexcept
{
    SI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SI_NetLine::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mStartPoint == nullptr) return false;
    if (mEndPoint == nullptr)   return false;
    if (mWidth < 0)             return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
