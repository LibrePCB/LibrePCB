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
#include "bi_footprint.h"
//#include "bi_footprintpad.h"
#include "../board.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../library/projectlibrary.h"
#include "../../../library/fpt/footprint.h"
#include "../../../library/pkg/package.h"
#include "../../../common/file_io/xmldomelement.h"
#include "../../../common/graphics/graphicsscene.h"
#include "../componentinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Footprint::BI_Footprint(ComponentInstance& component, const XmlDomElement& domElement) throw (Exception) :
    BI_Base(), mComponentInstance(component), mFootprint(nullptr),
    mGraphicsItem(nullptr)
{
    Q_UNUSED(domElement);
    init();
}

void BI_Footprint::init() throw (Exception)
{
    QUuid footprintUuid = mComponentInstance.getLibPackage().getFootprintUuid();
    mFootprint = mComponentInstance.getBoard().getProject().getLibrary().getFootprint(footprintUuid);
    if (!mFootprint)
    {
        throw RuntimeError(__FILE__, __LINE__, footprintUuid.toString(),
            QString(tr("No footprint with the UUID \"%1\" found in the project's library."))
            .arg(footprintUuid.toString()));
    }

    mGraphicsItem = new BGI_Footprint(*this);
    mGraphicsItem->setPos(20, 30);
    //mGraphicsItem->setPos(mPosition.toPxQPointF());
    //mGraphicsItem->setRotation(mAngle.toDeg());

    /*foreach (const library::SymbolPin* libPin, mSymbol->getPins())
    {
        SI_SymbolPin* pin = new SI_SymbolPin(*this, libPin->getUuid());
        if (mPins.contains(libPin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, libPin->getUuid().toString(),
                QString(tr("The symbol pin UUID \"%1\" is defined mulibPinple times."))
                .arg(libPin->getUuid().toString()));
        }
        if (!mSymbVarItem->getPinSignalMap().contains(libPin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, libPin->getUuid().toString(),
                QString(tr("Symbol pin UUID \"%1\" not found in pin-slibPinal-map."))
                .arg(libPin->getUuid().toString()));
        }
        mPins.insert(libPin->getUuid(), pin);
    }
    if (mPins.count() != mSymbVarItem->getPinSignalMap().count())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mPins.count()).arg(mSymbVarItem->getPinSignalMap().count()),
            QString(tr("The pin count of the symbol instance \"%1\" does not match with "
            "the pin-signal-map")).arg(mUuid.toString()));
    }*/

    // connect to the "attributes changes" signal of schematic and generic component
    //connect(mGenCompInstance, &GenCompInstance::attributesChanged,
    //        this, &SI_Symbol::schematicOrGenCompAttributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_Footprint::~BI_Footprint() noexcept
{
    //qDeleteAll(mPads);              mPads.clear();
    delete mGraphicsItem;           mGraphicsItem = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_Footprint::addToBoard(GraphicsScene& scene) throw (Exception)
{
    scene.addItem(*mGraphicsItem);
    //foreach (SI_SymbolPin* pin, mPins)
    //    pin->addToSchematic(scene);
}

void BI_Footprint::removeFromBoard(GraphicsScene& scene) throw (Exception)
{
    scene.removeItem(*mGraphicsItem);
    //foreach (SI_SymbolPin* pin, mPins)
    //    pin->removeFromSchematic(scene);
}

XmlDomElement* BI_Footprint::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("footprint"));
    /*root->setAttribute("uuid", mUuid);
    root->setAttribute("gen_comp_instance", mGenCompInstance->getUuid());
    root->setAttribute("symbol_item", mSymbVarItem->getUuid());
    XmlDomElement* position = root->appendChild("position");
    position->setAttribute("x", mPosition.getX());
    position->setAttribute("y", mPosition.getY());
    position->setAttribute("angle", mAngle);*/
    return root.take();
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

/*Point BI_Footprint::mapToScene(const Point& relativePos) const noexcept
{
    return (mPosition + relativePos).rotated(mAngle, mPosition);
}*/

/*bool SI_Symbol::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                       bool passToParents, QString& value) const noexcept
{
    if ((attrNS == QLatin1String("SYM")) || (attrNS.isEmpty()))
    {
        if (attrKey == QLatin1String("NAME"))
            return value = getName(), true;
    }

    if ((attrNS != QLatin1String("SYM")) && (passToParents))
    {
        if (mGenCompInstance->getAttributeValue(attrNS, attrKey, false, value))
            return true;
        if (mSchematic.getAttributeValue(attrNS, attrKey, true, value))
            return true;
    }

    return false;
}*/

/*****************************************************************************************
 *  Inherited from SI_Base
 ****************************************************************************************/

QPainterPath BI_Footprint::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void BI_Footprint::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
    //foreach (SI_SymbolPin* pin, mPins)
    //    pin->setSelected(selected);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool BI_Footprint::checkAttributesValidity() const noexcept
{
    /*if (mSymbVarItem == nullptr)        return false;
    if (mSymbol == nullptr)             return false;
    if (mUuid.isNull())                 return false;
    if (mGenCompInstance == nullptr)    return false;*/
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
