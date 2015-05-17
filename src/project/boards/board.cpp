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
#include "board.h"
#include "../../common/file_io/smartxmlfile.h"
#include "../../common/file_io/xmldomdocument.h"
#include "../../common/file_io/xmldomelement.h"
#include "../project.h"
#include "../../common/graphics/graphicsview.h"
#include "../../common/graphics/graphicsscene.h"
#include "../../common/gridproperties.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Board::Board(Project& project, const FilePath& filepath, bool restore,
             bool readOnly, bool create, const QString& newName) throw (Exception):
    QObject(nullptr), IF_AttributeProvider(), mProject(project), mFilePath(filepath),
    mXmlFile(nullptr), mAddedToProject(false), mGraphicsScene(nullptr),
    mGridProperties(nullptr)
{
    try
    {
        mGraphicsScene = new GraphicsScene();

        // try to open/create the XML board file
        if (create)
        {
            mXmlFile = SmartXmlFile::create(mFilePath);

            // set attributes
            mUuid = QUuid::createUuid();
            mName = newName;

            // load default grid properties
            mGridProperties = new GridProperties();
        }
        else
        {
            mXmlFile = new SmartXmlFile(mFilePath, restore, readOnly);
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
            XmlDomElement& root = doc->getRoot();

            // the board seems to be ready to open, so we will create all needed objects

            mUuid = root.getFirstChild("meta/uuid", true, true)->getText<QUuid>();
            mName = root.getFirstChild("meta/name", true, true)->getText(true);

            // Load grid properties
            mGridProperties = new GridProperties(*root.getFirstChild("properties/grid_properties", true, true));
        }

        updateIcon();

        // emit the "attributesChanged" signal when the project has emited it
        connect(&mProject, &Project::attributesChanged, this, &Board::attributesChanged);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        delete mGridProperties;         mGridProperties = nullptr;
        delete mXmlFile;                mXmlFile = nullptr;
        delete mGraphicsScene;          mGraphicsScene = nullptr;
        throw; // ...and rethrow the exception
    }
}

Board::~Board()
{
    delete mGridProperties;         mGridProperties = nullptr;
    delete mXmlFile;                mXmlFile = nullptr;
    delete mGraphicsScene;          mGraphicsScene = nullptr;
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

bool Board::isEmpty() const noexcept
{
    return false;
}

/*****************************************************************************************
 *  Setters: General
 ****************************************************************************************/

void Board::setGridProperties(const GridProperties& grid) noexcept
{
    *mGridProperties = grid;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Board::addToProject() throw (Exception)
{
    Q_ASSERT(mAddedToProject == false);
    mAddedToProject = true;
}

void Board::removeFromProject() throw (Exception)
{
    Q_ASSERT(mAddedToProject == true);
    mAddedToProject = false;
}

bool Board::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // save board XML file
    try
    {
        if (mAddedToProject)
        {
            XmlDomElement* root = serializeToXmlDomElement();
            XmlDomDocument doc(*root, true);
            mXmlFile->save(doc, toOriginal);
        }
        else
        {
            mXmlFile->removeFile(toOriginal);
        }
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    return success;
}

void Board::showInView(GraphicsView& view) noexcept
{
    view.setScene(mGraphicsScene);
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool Board::getAttributeValue(const QString& attrNS, const QString& attrKey,
                              bool passToParents, QString& value) const noexcept
{
    // TODO
    Q_UNUSED(attrNS);
    Q_UNUSED(attrKey);
    Q_UNUSED(passToParents);
    Q_UNUSED(value);
    return false;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Board::updateIcon() noexcept
{
    QRectF source = mGraphicsScene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
    QRect target(0, 0, 297, 210); // DIN A4 format :-)

    QPixmap pixmap(target.size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    mGraphicsScene->render(&painter, target, source);
    mIcon = QIcon(pixmap);
}

bool Board::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mName.isEmpty())    return false;
    return true;
}

XmlDomElement* Board::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("board"));
    XmlDomElement* meta = root->appendChild("meta");
    meta->appendTextChild("uuid", mUuid);
    meta->appendTextChild("name", mName);
    XmlDomElement* properties = root->appendChild("properties");
    properties->appendChild(mGridProperties->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Board* Board::create(Project& project, const FilePath& filepath,
                         const QString& name) throw (Exception)
{
    return new Board(project, filepath, false, false, true, name);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
