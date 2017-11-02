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
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/sexpression.h>

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
    mFilepath(project.getPath().getPathTo("core/erc.lp")), mFile(nullptr)
{
    // try to create/open the file "erc.lp"
    if (create) {
        mFile.reset(SmartSExprFile::create(mFilepath));
    } else {
        mFile.reset(new SmartSExprFile(mFilepath, restore, readOnly));
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
    if (mFile->isCreated()) return; // the file does not yet exist

    SExpression root = mFile->parseFileAndBuildDomTree();

    // reset all ignore attributes
    foreach (ErcMsg* ercMsg, mItems)
        ercMsg->setIgnored(false);

    // scan approved items and set ignore attributes
    foreach (const SExpression& node, root.getChildren("approved")) {
        foreach (ErcMsg* ercMsg, mItems) {
            if ((ercMsg->getOwner().getErcMsgOwnerClassName() == node.getValueByPath<QString>("class", false))
             && (ercMsg->getOwnerKey() == node.getValueByPath<QString>("instance", false))
             && (ercMsg->getMsgKey() == node.getValueByPath<QString>("message", false)))
            {
                ercMsg->setIgnored(true);
            }
        }
    }
}

bool ErcMsgList::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "core/erc.lp"
    try
    {
        SExpression doc(serializeToDomElement("librepcb_erc"));
        mFile->save(doc, toOriginal);
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

void ErcMsgList::serialize(SExpression& root) const
{
    foreach (ErcMsg* ercMsg, mItems) {
        if (ercMsg->isIgnored()) {
            SExpression& itemNode = root.appendList("approved", true);
            itemNode.appendStringChild("class", ercMsg->getOwner().getErcMsgOwnerClassName(), true);
            itemNode.appendStringChild("instance", ercMsg->getOwnerKey(), true);
            itemNode.appendStringChild("message", ercMsg->getMsgKey(), true);
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
