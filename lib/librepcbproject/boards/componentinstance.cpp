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
#include "componentinstance.h"
#include "board.h"
#include "../project.h"
#include "../library/projectlibrary.h"
#include <librepcblibrary/cmp/component.h>
#include <librepcblibrary/gencmp/genericcomponent.h>
#include "../erc/ercmsg.h"
#include <librepcbcommon/fileio/xmldomelement.h>
#include "../circuit/circuit.h"
#include "../circuit/gencompinstance.h"
#include "items/bi_footprint.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentInstance::ComponentInstance(Board& board, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mBoard(board), mAddedToBoard(false), mGenCompInstance(nullptr),
    mComponent(nullptr), mFootprint(nullptr)
{
    // get generic component instance
    QUuid genCompInstUuid = domElement.getAttribute<QUuid>("generic_component_instance");
    mGenCompInstance = mBoard.getProject().getCircuit().getGenCompInstanceByUuid(genCompInstUuid);
    if (!mGenCompInstance)
    {
        throw RuntimeError(__FILE__, __LINE__, genCompInstUuid.toString(),
            QString(tr("Could not found the generic component instance with UUID \"%1\"!"))
            .arg(genCompInstUuid.toString()));
    }
    // get component
    QUuid componentUuid = domElement.getAttribute<QUuid>("component");
    initComponentAndPackage(componentUuid);

    // get position and rotation
    mPosition.setX(domElement.getFirstChild("position", true)->getAttribute<Length>("x"));
    mPosition.setY(domElement.getFirstChild("position", true)->getAttribute<Length>("y"));
    mRotation = domElement.getFirstChild("position", true)->getAttribute<Angle>("rotation");

    // load footprint
    mFootprint = new BI_Footprint(*this, *domElement.getFirstChild("footprint", true));

    init();
}

ComponentInstance::ComponentInstance(Board& board, GenCompInstance& genCompInstance,
                                     const QUuid& componentUuid, const Point& position,
                                     const Angle& rotation) throw (Exception) :
    QObject(nullptr), mBoard(board), mAddedToBoard(false), mGenCompInstance(&genCompInstance),
    mComponent(nullptr), mFootprint(nullptr), mPosition(position), mRotation(rotation)
{
    initComponentAndPackage(componentUuid);

    // create footprint
    mFootprint = new BI_Footprint(*this);

    init();
}

void ComponentInstance::initComponentAndPackage(const QUuid& componentUuid) throw (Exception)
{
    // get component from library
    mComponent = mBoard.getProject().getLibrary().getComponent(componentUuid);
    if (!mComponent)
    {
        throw RuntimeError(__FILE__, __LINE__, componentUuid.toString(),
            QString(tr("No component with the UUID \"%1\" found in the project's library."))
            .arg(componentUuid.toString()));
    }
    // check if the component matches with the generic component
    if (mComponent->getGenCompUuid() != mGenCompInstance->getGenComp().getUuid())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The component \"%1\" does not match with the generic component"
            "instance \"%2\".")).arg(mComponent->getGenCompUuid().toString(),
            mGenCompInstance->getGenComp().getUuid().toString()));
    }
    // get package from library
    QUuid packageUuid = mComponent->getPackageUuid();
    mPackage = mBoard.getProject().getLibrary().getPackage(packageUuid);
    if (!mPackage)
    {
        throw RuntimeError(__FILE__, __LINE__, packageUuid.toString(),
            QString(tr("No package with the UUID \"%1\" found in the project's library."))
            .arg(packageUuid.toString()));
    }
}

void ComponentInstance::init() throw (Exception)
{
    // check pad-signal-map
    foreach (const QUuid& signalUuid, mComponent->getPadSignalMap())
    {
        if ((!signalUuid.isNull()) && (!mGenCompInstance->getSignalInstance(signalUuid)))
        {
            throw RuntimeError(__FILE__, __LINE__, signalUuid.toString(),
                QString(tr("Unknown signal \"%1\" found in component \"%2\""))
                .arg(signalUuid.toString(), mComponent->getUuid().toString()));
        }
    }

    // emit the "attributesChanged" signal when the project has emited it
    //connect(&mCircuit.getProject(), &Project::attributesChanged, this, &ComponentInstance::attributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentInstance::~ComponentInstance() noexcept
{
    Q_ASSERT(!mAddedToBoard);
    delete mFootprint;          mFootprint = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Project& ComponentInstance::getProject() const noexcept
{
    return mBoard.getProject();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentInstance::setPosition(const Point& pos) noexcept
{
    mPosition = pos;
    emit moved(mPosition);
}

void ComponentInstance::setRotation(const Angle& rot) noexcept
{
    mRotation = rot;
    emit rotated(mRotation);
}

void ComponentInstance::addToBoard(GraphicsScene& scene) throw (Exception)
{
    if (mAddedToBoard) throw LogicError(__FILE__, __LINE__);
    mGenCompInstance->registerComponent(*this);
    mFootprint->addToBoard(scene);
    mAddedToBoard = true;
    updateErcMessages();
}

void ComponentInstance::removeFromBoard(GraphicsScene& scene) throw (Exception)
{
    if (!mAddedToBoard) throw LogicError(__FILE__, __LINE__);
    mGenCompInstance->unregisterComponent(*this);
    mFootprint->removeFromBoard(scene);
    mAddedToBoard = false;
    updateErcMessages();
}

XmlDomElement* ComponentInstance::serializeToXmlDomElement(uint version) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("component_instance"));
    root->setAttribute("generic_component_instance", mGenCompInstance->getUuid());
    root->setAttribute("component", mComponent->getUuid());
    root->appendChild(mFootprint->serializeToXmlDomElement(version));
    XmlDomElement* position = root->appendChild("position");
    position->setAttribute("x", mPosition.getX());
    position->setAttribute("y", mPosition.getY());
    position->setAttribute("rotation", mRotation);
    return root.take();
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool ComponentInstance::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                        bool passToParents, QString& value) const noexcept
{
    // no local attributes available

    if (((attrNS == QLatin1String("CMP")) || (attrNS.isEmpty())) && passToParents)
    {
        if (mGenCompInstance->getAttributeValue(attrNS, attrKey, false, value))
            return true;
    }

    if ((attrNS != QLatin1String("CMP")) && (passToParents))
        return mBoard.getAttributeValue(attrNS, attrKey, true, value);
    else
        return false;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentInstance::checkAttributesValidity() const noexcept
{
    if (mGenCompInstance == nullptr)            return false;
    if (mComponent == nullptr)                  return false;
    if (mPackage == nullptr)                    return false;
    return true;
}

void ComponentInstance::updateErcMessages() noexcept
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
