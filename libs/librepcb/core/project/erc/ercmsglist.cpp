/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "ercmsglist.h"

#include "../../serialization/sexpression.h"
#include "../project.h"
#include "ercmsg.h"
#include "if_ercmsgprovider.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ErcMsgList::ErcMsgList(Project& project)
  : QObject(&project), mProject(project) {
}

ErcMsgList::~ErcMsgList() noexcept {
  Q_ASSERT(mItems.isEmpty());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ErcMsgList::add(ErcMsg* ercMsg) noexcept {
  Q_ASSERT(ercMsg);
  Q_ASSERT(!mItems.contains(ercMsg));
  Q_ASSERT(!ercMsg->isIgnored());
  mItems.append(ercMsg);
  emit ercMsgAdded(ercMsg);
}

void ErcMsgList::remove(ErcMsg* ercMsg) noexcept {
  Q_ASSERT(ercMsg);
  Q_ASSERT(mItems.contains(ercMsg));
  Q_ASSERT(!ercMsg->isIgnored());
  mItems.removeOne(ercMsg);
  emit ercMsgRemoved(ercMsg);
}

void ErcMsgList::update(ErcMsg* ercMsg) noexcept {
  Q_ASSERT(ercMsg);
  Q_ASSERT(mItems.contains(ercMsg));
  Q_ASSERT(ercMsg->isVisible());
  emit ercMsgChanged(ercMsg);
}

void ErcMsgList::serialize(SExpression& root) const {
  foreach (const ErcMsg* msg, mItems) {
    if (msg->isIgnored()) {
      root.ensureLineBreak();
      SExpression& node = root.appendList("approved");
      node.ensureLineBreak();
      node.appendChild("class",
                       QString(msg->getOwner().getErcMsgOwnerClassName()));
      node.ensureLineBreak();
      node.appendChild("instance", msg->getOwnerKey());
      node.ensureLineBreak();
      node.appendChild("message", msg->getMsgKey());
      node.ensureLineBreak();
    }
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
