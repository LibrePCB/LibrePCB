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
#include "copyoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CopyOutputJob::CopyOutputJob() noexcept
  : OutputJob(getTypeName(), Uuid::createRandom(),
              elementNameFromTr("CopyOutputJob", QT_TR_NOOP("Custom File"))),
    mSubstituteVariables(false),
    mBoards(BoardSet::set({std::nullopt})),
    mAssemblyVariants(AssemblyVariantSet::set({std::nullopt})),
    mInputPath("resources/template.txt"),
    mOutputPath("{{PROJECT}}_{{VERSION}}.txt") {
}

CopyOutputJob::CopyOutputJob(const CopyOutputJob& other) noexcept
  : OutputJob(other),
    mSubstituteVariables(other.mSubstituteVariables),
    mBoards(other.mBoards),
    mAssemblyVariants(other.mAssemblyVariants),
    mInputPath(other.mInputPath),
    mOutputPath(other.mOutputPath) {
}

CopyOutputJob::CopyOutputJob(const SExpression& node)
  : OutputJob(node),
    mSubstituteVariables(
        deserialize<bool>(node.getChild("substitute_variables/@0"))),
    mBoards(node, "board"),
    mAssemblyVariants(node, "variant"),
    mInputPath(node.getChild("input/@0").getValue()),
    mOutputPath(node.getChild("output/@0").getValue()) {
}

CopyOutputJob::~CopyOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString CopyOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon CopyOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/copy.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CopyOutputJob::setSubstituteVariables(bool subst) noexcept {
  if (subst != mSubstituteVariables) {
    mSubstituteVariables = subst;
    onEdited.notify(Event::PropertyChanged);
  }
}

void CopyOutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void CopyOutputJob::setAssemblyVariants(
    const AssemblyVariantSet& avs) noexcept {
  if (avs != mAssemblyVariants) {
    mAssemblyVariants = avs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void CopyOutputJob::setInputPath(const QString& path) noexcept {
  if (path != mInputPath) {
    mInputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

void CopyOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> CopyOutputJob::cloneShared() const noexcept {
  return std::make_shared<CopyOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CopyOutputJob::serializeDerived(SExpression& root) const {
  root.appendChild("substitute_variables", mSubstituteVariables);
  root.ensureLineBreak();
  mBoards.serialize(root, "board");
  root.ensureLineBreak();
  mAssemblyVariants.serialize(root, "variant");
  root.ensureLineBreak();
  root.appendChild("input", mInputPath);
  root.ensureLineBreak();
  root.appendChild("output", mOutputPath);
}

bool CopyOutputJob::equals(const OutputJob& rhs) const noexcept {
  const CopyOutputJob& other = static_cast<const CopyOutputJob&>(rhs);
  if (mOutputPath != other.mOutputPath) return false;
  if (mInputPath != other.mInputPath) return false;
  if (mAssemblyVariants != other.mAssemblyVariants) return false;
  if (mBoards != other.mBoards) return false;
  if (mSubstituteVariables != other.mSubstituteVariables) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
