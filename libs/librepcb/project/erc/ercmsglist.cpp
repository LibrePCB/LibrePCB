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
#include <librepcb/common/fileio/domdocument.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ErcMsgList::ErcMsgList(Project& project, bool restore, bool readOnly, bool create) :
    QObject(&project), mProject(project),
    mXmlFilepath(project.getPath().getPathTo("core/erc.xml")), mXmlFile(nullptr)
{
    // try to create/open the XML file "erc.xml"
    if (create) {
        mXmlFile.reset(SmartXmlFile::create(mXmlFilepath));
    } else {
        mXmlFile.reset(new SmartXmlFile(mXmlFilepath, restore, readOnly));
    }
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

void ErcMsgList::restoreIgnoreState()
{
    if (mXmlFile->isCreated()) return; // the XML file does not yet exist

    std::unique_ptr<DomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
    DomElement& root = doc->getRoot();

    // reset all ignore attributes
    foreach (ErcMsg* ercMsg, mItems)
        ercMsg->setIgnored(false);

    // scan ignored items and set ignore attributes
    foreach (const DomElement* node, root.getChilds("ignore")) {
        foreach (ErcMsg* ercMsg, mItems) {
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
        DomDocument doc(*serializeToDomElement("erc"));
        mXmlFile->save(doc, toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ErcMsgList::serialize(DomElement& root) const
{
    foreach (ErcMsg* ercMsg, mItems) {
        if (ercMsg->isIgnored()) {
            DomElement* itemNode = root.appendChild("ignore");
            itemNode->setAttribute("owner_class", ercMsg->getOwner().getErcMsgOwnerClassName());
            itemNode->setAttribute("owner_key", ercMsg->getOwnerKey());
            itemNode->setAttribute("msg_key", ercMsg->getMsgKey());
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
