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
#include "schematic.h"
#include "../../common/xmlfile.h"
#include "../project.h"
#include "symbolinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Schematic::Schematic(Project& project, const FilePath& filepath, bool restore)
                     throw (Exception):
    CADScene(), mProject(project), mFilePath(filepath), mXmlFile(0)
{
    try
    {
        // try to open the XML schematic file
        mXmlFile = new XmlFile(mFilePath, restore, "schematic");

        // the schematic seems to be ready to open, so we will create all needed objects

        QDomElement tmpNode; // for temporary use

        tmpNode = mXmlFile->getRoot().firstChildElement("meta");
        mUuid = tmpNode.firstChildElement("uuid").text();
        if(mUuid.isNull())
        {
            throw RuntimeError(__FILE__, __LINE__, tmpNode.firstChildElement("uuid").text(),
                QString(tr("Invalid schematic UUID: \"%1\""))
                .arg(tmpNode.firstChildElement("uuid").text()));
        }

        mName = tmpNode.firstChildElement("name").text();
        if(mName.isEmpty())
        {
            throw RuntimeError(__FILE__, __LINE__, mUuid.toString(),
                QString(tr("Name of schematic \"%1\" is empty!")).arg(mUuid.toString()));
        }

        // Load all symbol instances
        tmpNode = mXmlFile->getRoot().firstChildElement("symbols").firstChildElement("symbol");
        while (!tmpNode.isNull())
        {
            SymbolInstance* symbol = new SymbolInstance(*this, tmpNode);
            mSymbols.insert(symbol->getUuid(), symbol);
            tmpNode = tmpNode.nextSiblingElement("symbol");
        }

        updateIcon();
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        qDeleteAll(mSymbols);           mSymbols.clear();
        delete mXmlFile;                mXmlFile = 0;
        throw; // ...and rethrow the exception
    }
}

Schematic::~Schematic()
{
    qDeleteAll(mSymbols);           mSymbols.clear();
    delete mXmlFile;                mXmlFile = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Schematic::removeFiles() const throw (Exception)
{
    mXmlFile->remove();
}

bool Schematic::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

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
 *  Private Methods
 ****************************************************************************************/

void Schematic::updateIcon() noexcept
{
    QRectF rect;
    if (items().count() > 0)
        rect = itemsBoundingRect();
    else
        rect = QRectF(0, 0, 300, 200);

    QPixmap pixmap(rect.size().toSize());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    render(&painter, QRectF(), rect);
    mIcon = QIcon(pixmap);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Schematic* Schematic::create(Project& project, const FilePath& filepath,
                             const QString& name) throw (Exception)
{
    // create XML file with root node
    XmlFile* file = XmlFile::create(filepath, "schematic");

    // create node "meta" with schematic UUID and name
    QDomElement metaNode = file->getDocument().createElement("meta");
    QDomElement uuidNode = file->getDocument().createElement("uuid");
    QDomElement nameNode = file->getDocument().createElement("name");
    QDomText uuidText = file->getDocument().createTextNode(QUuid::createUuid().toString());
    QDomText nameText = file->getDocument().createTextNode(name);
    uuidNode.appendChild(uuidText);
    nameNode.appendChild(nameText);
    metaNode.appendChild(uuidNode);
    metaNode.appendChild(nameNode);
    file->getRoot().appendChild(metaNode);

    try
    {
        file->save(false); // write new (temporary) XML file to filesystem
        Schematic* schematic = new Schematic(project, filepath, true); // create new Schematic
        delete file; // this will remove the temporary file, so don't do this earlier!
        return schematic;
    }
    catch (Exception& e)
    {
        delete file;
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
