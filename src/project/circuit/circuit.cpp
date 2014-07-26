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
#include "circuit.h"
#include "../project.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Circuit::Circuit(Workspace* workspace, Project* project) :
    QObject(0), mWorkspace(workspace), mProject(project),
    mDomDocument(0), mRootDomElement(0)
{
    // get the absolute filepath to "core/circuit.xml"
    mXmlFilepath = QDir::toNativeSeparators(mProject->getDir().absoluteFilePath("core/circuit.xml"));

    try
    {
        // load the XML file in "mDomDocument" and create "mRootDomElement"
        QFile xmlFile(mXmlFilepath);
        if (!xmlFile.exists())
            throw RuntimeError(QString("File \"%1\" not found!").arg(mXmlFilepath), __FILE__, __LINE__);
        if (!xmlFile.open(QIODevice::ReadOnly))
            throw RuntimeError(QString("Cannot open file \"%1\"!").arg(mXmlFilepath), __FILE__, __LINE__);
        mDomDocument = new QDomDocument();
        if (!mDomDocument->setContent(&xmlFile))
            throw RuntimeError(QString("Cannot load file \"%1\"!").arg(mXmlFilepath), __FILE__, __LINE__);
        xmlFile.close();
        mRootDomElement = new QDomElement(mDomDocument->firstChildElement("circuit"));
        if (mRootDomElement->isNull())
            throw RuntimeError(QString("Invalid XML in \"%1\"!").arg(mXmlFilepath), __FILE__, __LINE__);

        // all ok, now read the XML content

        mUuid = mRootDomElement->firstChildElement("meta").firstChildElement("uuid").text();
        if (mUuid.isNull())
            throw RuntimeError(QString("Invalid circuit UUID!"), __FILE__, __LINE__);
    }
    catch (...)
    {
        // free allocated memory and rethrow the exception
        delete mRootDomElement;
        delete mDomDocument;
        throw;
    }
}

Circuit::~Circuit()
{
    delete mRootDomElement;     mRootDomElement = 0;
    delete mDomDocument;        mDomDocument = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Circuit::save()
{
    // Save "core/circuit.xml"
    QFile xmlFile(mXmlFilepath);
    if (!xmlFile.exists())
        throw RuntimeError(QString("File \"%1\" not found!").arg(mXmlFilepath), __FILE__, __LINE__);
    if (!xmlFile.open(QIODevice::WriteOnly))
        throw RuntimeError(QString("Cannot open file \"%1\"!").arg(mXmlFilepath), __FILE__, __LINE__);
    QTextStream stream(&xmlFile);
    stream << mDomDocument->toString(4);
    xmlFile.close();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
