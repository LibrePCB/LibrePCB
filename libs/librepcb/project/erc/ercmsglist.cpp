/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/xmldomdocument.h>
#include <librepcb/common/fileio/xmldomelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ErcMsgList::ErcMsgList(Project& project, bool restore, bool readOnly, bool create) throw (Exception) :
    QObject(&project), mProject(project),
    mXmlFilepath(project.getPath().getPathTo("core/erc.xml")), mXmlFile(nullptr)
{
    // try to create/open the XML file "erc.xml"
    if (create) {
        mXmlFile.reset(SmartXmlFile::create(mXmlFilepath));
    } else {
        mXmlFile.reset(new SmartXmlFile(mXmlFilepath, restore, readOnly));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ErcMsgList::~ErcMsgList() noexcept
{
    Q_ASSERT(mItems.isEmpty());
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

    QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
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
            if ((ercMsg->getOwner().getErcMsgOwnerClassName() == node->getAttribute<QString>("owner_class", false))
             && (ercMsg->getOwnerKey() == node->getAttribute<QString>("owner_key", false))
             && (ercMsg->getMsgKey() == node->getAttribute<QString>("msg_key", false)))
            {
                ercMsg->setIgnored(true);
            }
        }
    }
}

bool ErcMsgList::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "core/erc.xml"
    try
    {
        XmlDomDocument doc(*serializeToXmlDomElement());
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

XmlDomElement* ErcMsgList::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("erc"));
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
} // namespace librepcb
