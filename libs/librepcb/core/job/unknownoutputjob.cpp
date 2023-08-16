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
#include "unknownoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UnknownOutputJob::UnknownOutputJob(const UnknownOutputJob& other) noexcept
  : OutputJob(other), mNode(other.mNode) {
}

UnknownOutputJob::UnknownOutputJob(const SExpression& node)
  : OutputJob(node), mNode(node) {
}

UnknownOutputJob::~UnknownOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString UnknownOutputJob::getTypeTr() const noexcept {
  return tr("Unknown") % " (" % mType % ")";
}

QIcon UnknownOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/status/dialog_error.png");
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> UnknownOutputJob::cloneShared() const noexcept {
  return std::make_shared<UnknownOutputJob>(*this);
}

void UnknownOutputJob::serialize(SExpression& root) const {
  root = mNode;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void UnknownOutputJob::serializeDerived(SExpression& root) const {
  Q_UNUSED(root);
}

bool UnknownOutputJob::equals(const OutputJob& rhs) const noexcept {
  const UnknownOutputJob& other = static_cast<const UnknownOutputJob&>(rhs);
  if (mNode != other.mNode) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
