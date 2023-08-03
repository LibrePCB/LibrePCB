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
#include "netlistoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetlistOutputJob::NetlistOutputJob() noexcept
  : OutputJob(getTypeName(), Uuid::createRandom(),
              elementNameFromTr("NetlistOutputJob", QT_TR_NOOP("Netlist"))),
    mBoards(BoardSet::onlyDefault()),
    mOutputPath("{{PROJECT}}_{{VERSION}}_Netlist.d356") {
}

NetlistOutputJob::NetlistOutputJob(const NetlistOutputJob& other) noexcept
  : OutputJob(other), mBoards(other.mBoards), mOutputPath(other.mOutputPath) {
}

NetlistOutputJob::NetlistOutputJob(const SExpression& node)
  : OutputJob(node),
    mBoards(node, "board"),
    mOutputPath(node.getChild("output/@0").getValue()) {
}

NetlistOutputJob::~NetlistOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString NetlistOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon NetlistOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/places/file.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NetlistOutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void NetlistOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> NetlistOutputJob::cloneShared() const noexcept {
  return std::make_shared<NetlistOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NetlistOutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  mBoards.serialize(root, "board");
  root.ensureLineBreak();
  root.appendChild("output", mOutputPath);
}

bool NetlistOutputJob::equals(const OutputJob& rhs) const noexcept {
  const NetlistOutputJob& other = static_cast<const NetlistOutputJob&>(rhs);
  if (mBoards != other.mBoards) return false;
  if (mOutputPath != other.mOutputPath) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
