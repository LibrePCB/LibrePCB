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
#include <QDomDocument>
#include "../../common/exceptions.h"
#include "../../common/xmlfile.h"
#include "circuit.h"
#include "../project.h"
#include "netclass.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Circuit::Circuit(Workspace& workspace, Project& project, bool restore) throw (Exception) :
    QObject(0), mWorkspace(workspace), mProject(project),
    mXmlFilepath(project.getPath().getPathTo("core/circuit.xml")), mXmlFile(0)
{
    qDebug() << "load circuit...";

    try
    {
        // try to open the XML file "circuit.xml"
        mXmlFile = new XmlFile(mXmlFilepath, restore, "circuit");

        // OK - XML file is open --> now load the whole circuit stuff

        QDomElement tmpNode; // for temporary use...
        QDomElement root = mXmlFile->getRoot();

        // Load all netclasses
        tmpNode = root.firstChildElement("netclasses");
        NetClass::loadFromCircuit(mWorkspace, mProject, *this, tmpNode, mNetClasses);
    }
    catch (...)
    {
        // free allocated memory and rethrow the exception
        qDeleteAll(mNetClasses);    mNetClasses.clear();
        delete mXmlFile;            mXmlFile = 0;
        throw;
    }

    qDebug() << "circuit successfully loaded!";
}

Circuit::~Circuit() noexcept
{
    qDeleteAll(mNetClasses);    mNetClasses.clear();
    delete mXmlFile;            mXmlFile = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool Circuit::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "core/circuit.xml"
    try
    {
        mXmlFile->save(toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    return success;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
