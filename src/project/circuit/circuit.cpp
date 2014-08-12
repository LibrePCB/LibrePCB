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

Circuit::Circuit(Workspace& workspace, Project& project, bool restore) throw (Exception) :
    QObject(0), mWorkspace(workspace), mProject(project), mDomDocument(0),
    mRootDomElement(0)
{
    // get the absolute filepath to "core/circuit.xml" (UNIX style)
    //mXmlFilepath = mProject->getDir().filePath("core/circuit.xml");

    try
    {
        // TODO

        // load the XML file in "mDomDocument" and create "mRootDomElement"
        /*QFile xmlFile(mXmlFilepath);
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
            throw RuntimeError(QString("Invalid XML in \"%1\"!").arg(mXmlFilepath), __FILE__, __LINE__);*/

        // all ok, now read the XML content

        // TODO
    }
    catch (...)
    {
        // free allocated memory and rethrow the exception
        delete mDomDocument;
        delete mRootDomElement;
        throw;
    }
}

Circuit::~Circuit() noexcept
{
    delete mRootDomElement;     mRootDomElement = 0;
    delete mDomDocument;        mDomDocument = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool Circuit::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;
    /*QString tilde = toOriginal ? "" : "~";

    // Save "core/circuit.xml"
    QFile xmlFile(mXmlFilepath % tilde);
    {
        success = false;
        errors.append(QString(tr("Could not open the file \"%1\"!")).arg(xmlFile.fileName()));
        qCritical() << "Could not open the circuit file:" << xmlFile.fileName();
    }
    QByteArray fileContent = mDomDocument->toByteArray(4);
    if (xmlFile.write(fileContent) != fileContent.size())
    {
        success = false;
        errors.append(QString(tr("Could not write to the file \"%1\"!")).arg(xmlFile.fileName()));
        qCritical() << "Could not write to the circuit file:" << xmlFile.fileName();
    }
    xmlFile.close();*/

    return success;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
