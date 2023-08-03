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
#include "board3doutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Board3DOutputJob::Board3DOutputJob() noexcept
  : OutputJob(getTypeName(), Uuid::createRandom(),
              elementNameFromTr("Board3DOutputJob", QT_TR_NOOP("STEP Model"))),
    mBoards(BoardSet::onlyDefault()),
    mAssemblyVariants(AssemblyVariantSet::onlyDefault()),
    mOutputPath("{{PROJECT}}_{{VERSION}}.step") {
}

Board3DOutputJob::Board3DOutputJob(const Board3DOutputJob& other) noexcept
  : OutputJob(other),
    mBoards(other.mBoards),
    mAssemblyVariants(other.mAssemblyVariants),
    mOutputPath(other.mOutputPath) {
}

Board3DOutputJob::Board3DOutputJob(const SExpression& node)
  : OutputJob(node),
    mBoards(node, "board"),
    mAssemblyVariants(node, "variant"),
    mOutputPath(node.getChild("output/@0").getValue()) {
}

Board3DOutputJob::~Board3DOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString Board3DOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon Board3DOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/export_step.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Board3DOutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void Board3DOutputJob::setAssemblyVariants(
    const AssemblyVariantSet& avs) noexcept {
  if (avs != mAssemblyVariants) {
    mAssemblyVariants = avs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void Board3DOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> Board3DOutputJob::cloneShared() const noexcept {
  return std::make_shared<Board3DOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Board3DOutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  mBoards.serialize(root, "board");
  root.ensureLineBreak();
  mAssemblyVariants.serialize(root, "variant");
  root.ensureLineBreak();
  root.appendChild("output", mOutputPath);
}

bool Board3DOutputJob::equals(const OutputJob& rhs) const noexcept {
  const Board3DOutputJob& other = static_cast<const Board3DOutputJob&>(rhs);
  if (mBoards != other.mBoards) return false;
  if (mAssemblyVariants != other.mAssemblyVariants) return false;
  if (mOutputPath != other.mOutputPath) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
