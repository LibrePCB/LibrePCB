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
#include "ercmsg.h"
#include "ercmsglist.h"
#include "../project.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ErcMsg::ErcMsg(Project& project, const IF_ErcMsgProvider& owner, const QString& ownerKey,
               const QString& msgKey, ErcMsgType_t msgType, const QString& msg) :
    mProject(project), mErcMsgList(project.getErcMsgList()), mOwner(owner),
    mOwnerKey(ownerKey), mMsgKey(msgKey), mMsgType(msgType), mMsg(msg),
    mIsVisible(false), mIsIgnored(false)
{
}

ErcMsg::~ErcMsg() noexcept
{
    setVisible(false);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ErcMsg::setMsg(const QString& msg) noexcept
{
    if (msg == mMsg) return;
    mMsg = msg;
    if (mIsVisible) mErcMsgList.update(this);
}

void ErcMsg::setVisible(bool visible) noexcept
{
    if (visible == mIsVisible) return;
    mIsVisible = visible;
    mIsIgnored = false; // changing the visibility will always reset the ignore flag!

    if (mIsVisible)
        mErcMsgList.add(this);
    else
        mErcMsgList.remove(this);
}

void ErcMsg::setIgnored(bool ignored, bool fromUserInput) noexcept
{
    if (ignored == mIsIgnored) return;
    mIsIgnored = ignored;
    mErcMsgList.update(this);
    if (fromUserInput) mProject.setModifiedFlag();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
