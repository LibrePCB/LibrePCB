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
#include "componentinstance.h"
#include "board.h"
#include "../project.h"
#include "../library/projectlibrary.h"
#include "../../library/cmp/component.h"
#include "../../library/gencmp/genericcomponent.h"
#include "../erc/ercmsg.h"
#include "../../common/file_io/xmldomelement.h"
#include "../circuit/circuit.h"
#include "../circuit/gencompinstance.h"
#include "items/bi_footprint.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentInstance::ComponentInstance(Board& board, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mBoard(board), mAddedToBoard(false), mComponent(nullptr),
    mFootprint(nullptr)
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
    // get component from library
    QUuid componentUuid = domElement.getAttribute<QUuid>("component");
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
    // load footprint
    mFootprint = new BI_Footprint(*this, *domElement.getFirstChild("footprint", true));

    init();
}

void ComponentInstance::init() throw (Exception)
{
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
 *  General Methods
 ****************************************************************************************/

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

XmlDomElement* ComponentInstance::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("component_instance"));
    root->setAttribute("generic_component_instance", mGenCompInstance->getUuid());
    root->setAttribute("component", mComponent->getUuid());
    return root.take();
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

/*bool ComponentInstance::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                        bool passToParents, QString& value) const noexcept
{
    if ((attrNS == QLatin1String("CMP")) || (attrNS.isEmpty()))
    {
        if (attrKey == QLatin1String("NAME"))
            return value = mName, true;
        else if (attrKey == QLatin1String("VALUE"))
            return value = mValue, true;
        else if (getAttributeByKey(attrKey))
            return value = getAttributeByKey(attrKey)->getValueTr(true), true;
    }

    if ((attrNS != QLatin1String("CMP")) && (passToParents))
        return mCircuit.getProject().getAttributeValue(attrNS, attrKey, passToParents, value);
    else
        return false;
}*/

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
