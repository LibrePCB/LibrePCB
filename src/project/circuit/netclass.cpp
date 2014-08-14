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
#include <QtWidgets>
#include "netclass.h"
#include "../../common/exceptions.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NetClass::NetClass(Workspace& workspace, Project& project, Circuit& circuit,
                   const QDomElement& domElement) :
    QObject(0), mWorkspace(workspace), mProject(project), mCircuit(circuit),
    mDomElement(domElement)
{
    mUuid = mDomElement.attribute("uuid");
    mName = mDomElement.attribute("name");
}

NetClass::~NetClass()
{
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

void NetClass::loadFromCircuit(Workspace& workspace, Project& project,
                               Circuit& circuit, const QDomElement& node,
                               QHash<QUuid, NetClass*>& list)
{
    qDebug() << "load netclasses...";
    QDomElement item = node.firstChildElement("netclass");
    while (!item.isNull())
    {
        NetClass* netclass = new NetClass(workspace, project, circuit, item);
        if (list.contains(netclass->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, netclass->getUuid().toString(),
                QString(tr("The netclass UUID %1 exists multiple times!"))
                .arg(netclass->getUuid().toString()));
        }
        list.insert(netclass->getUuid(), netclass);
        item = item.nextSiblingElement("netclass");
    }
    qDebug() << list.count() << "netclasses successfully loaded!";
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
