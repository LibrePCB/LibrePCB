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
#include "ercmsglist.h"
#include "ercmsg.h"
#include "if_ercmsgprovider.h"
#include "../project.h"
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ErcMsgList::ErcMsgList(Project& project, bool restore, bool readOnly, bool create) throw (Exception) :
    QObject(0), mProject(project),
    mXmlFilepath(project.getPath().getPathTo("core/erc.xml")), mXmlFile(nullptr)
{
    try
    {
        // try to create/open the XML file "erc.xml"
        if (create)
            mXmlFile = SmartXmlFile::create(mXmlFilepath);
        else
            mXmlFile = new SmartXmlFile(mXmlFilepath, restore, readOnly);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free allocated memory and rethrow the exception
        delete mXmlFile;            mXmlFile = nullptr;
        throw;
    }
}

ErcMsgList::~ErcMsgList() noexcept
{
    Q_ASSERT(mItems.isEmpty());
    delete mXmlFile;            mXmlFile = nullptr;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ErcMsgList::add(ErcMsg* ercMsg) noexcept
{
    Q_ASSERT(ercMsg);
    Q_ASSERT(!mItems.contains(ercMsg));
    Q_ASSERT(!ercMsg->isIgnored());
    mItems.append(ercMsg);
    emit ercMsgAdded(ercMsg);
}

void ErcMsgList::remove(ErcMsg* ercMsg) noexcept
{
    Q_ASSERT(ercMsg);
    Q_ASSERT(mItems.contains(ercMsg));
    Q_ASSERT(!ercMsg->isIgnored());
    mItems.removeOne(ercMsg);
    emit ercMsgRemoved(ercMsg);
}

void ErcMsgList::update(ErcMsg* ercMsg) noexcept
{
    Q_ASSERT(ercMsg);
    Q_ASSERT(mItems.contains(ercMsg));
    Q_ASSERT(ercMsg->isVisible());
    emit ercMsgChanged(ercMsg);
}

void ErcMsgList::restoreIgnoreState() noexcept
{
    if (mXmlFile->isCreated()) return; // the XML file does not yet exist

    QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree(true);
    XmlDomElement& root = doc->getRoot();

    // reset all ignore attributes
    foreach (ErcMsg* ercMsg, mItems)
        ercMsg->setIgnored(false);

    // scan ignored items and set ignore attributes
    for (XmlDomElement* node = root.getFirstChild("ignore/item", true, false);
         node; node = node->getNextSibling("item"))
    {
        foreach (ErcMsg* ercMsg, mItems)
        {
            if ((ercMsg->getOwner().getErcMsgOwnerClassName() == node->getAttribute("owner_class"))
             && (ercMsg->getOwnerKey() == node->getAttribute("owner_key"))
             && (ercMsg->getMsgKey() == node->getAttribute("msg_key")))
            {
                ercMsg->setIgnored(true);
            }
        }
    }
}

bool ErcMsgList::save(int version, bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "core/erc.xml"
    try
    {
        XmlDomDocument doc(*serializeToXmlDomElement(version));
        mXmlFile->save(doc, toOriginal);
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

bool ErcMsgList::checkAttributesValidity() const noexcept
{
    return true;
}

XmlDomElement* ErcMsgList::serializeToXmlDomElement(int version) const throw (Exception)
{
    Q_UNUSED(version);
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("erc"));
    root->setAttribute("version", version);
    XmlDomElement* ignoreNode = root->appendChild("ignore");
    foreach (ErcMsg* ercMsg, mItems)
    {
        if (ercMsg->isIgnored())
        {
            XmlDomElement* itemNode = ignoreNode->appendChild("item");
            itemNode->setAttribute("owner_class", ercMsg->getOwner().getErcMsgOwnerClassName());
            itemNode->setAttribute("owner_key", ercMsg->getOwnerKey());
            itemNode->setAttribute("msg_key", ercMsg->getMsgKey());
        }
    }
    return root.take();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
