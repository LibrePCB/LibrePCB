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
#include "ercmsglist.h"
#include "ercmsg.h"
#include "if_ercmsgprovider.h"
#include "../project.h"
#include "../../common/smartxmlfile.h"

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
            mXmlFile = SmartXmlFile::create(mXmlFilepath, "erc", 0);
        else
            mXmlFile = new SmartXmlFile(mXmlFilepath, restore, readOnly, "erc", 0);
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
    QDomElement child = mXmlFile->getRoot().firstChildElement("ignore");
    if (child.isNull()) return; // XML file is empty

    // reset all ignore attributes
    foreach (ErcMsg* ercMsg, mItems)
        ercMsg->setIgnored(false, false);

    // scan ignored items and set ignore attributes
    QDomElement item = child.firstChildElement("item");
    while (!item.isNull())
    {
        foreach (ErcMsg* ercMsg, mItems)
        {
            if ((ercMsg->getOwner().getErcMsgOwnerClassName() == item.attribute("owner_class"))
             && (ercMsg->getOwnerKey() == item.attribute("owner_key"))
             && (ercMsg->getMsgKey() == item.attribute("msg_key")))
            {
                ercMsg->setIgnored(true, false);
            }
        }
        item = item.nextSiblingElement("item");
    }
}

bool ErcMsgList::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "core/erc.xml"
    try
    {
        QDomElement root = mXmlFile->getRoot();

        // clear the file and create a new child
        root.removeChild(root.firstChildElement("ignore"));
        QDomElement child = mXmlFile->getDocument().createElement("ignore");
        if (child.isNull())
            throw RuntimeError(__FILE__, __LINE__, QString(), tr("XML DOM Error"));
        if (root.appendChild(child).isNull())
            throw RuntimeError(__FILE__, __LINE__, QString(), tr("XML DOM Error"));

        // add new entries
        foreach (ErcMsg* ercMsg, mItems)
        {
            if (ercMsg->isIgnored())
            {
                QDomElement node = mXmlFile->getDocument().createElement("item");
                if (node.isNull())
                    throw RuntimeError(__FILE__, __LINE__, QString(), tr("XML DOM Error"));
                node.setAttribute("owner_class", ercMsg->getOwner().getErcMsgOwnerClassName());
                node.setAttribute("owner_key", ercMsg->getOwnerKey());
                node.setAttribute("msg_key", ercMsg->getMsgKey());
                if (child.appendChild(node).isNull())
                    throw RuntimeError(__FILE__, __LINE__, QString(), tr("XML DOM Error"));
            }
        }

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
