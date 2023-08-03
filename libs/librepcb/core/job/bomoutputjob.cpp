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
#include "bomoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BomOutputJob::BomOutputJob() noexcept
  : OutputJob(
        getTypeName(), Uuid::createRandom(),
        elementNameFromTr("BomOutputJob", QT_TR_NOOP("Bill of Materials"))),
    mCustomAttributes(),
    mBoards(BoardSet::onlyDefault()),
    mAssemblyVariants(AssemblyVariantSet::all()),
    mOutputPath("assembly/{{PROJECT}}_{{VERSION}}_BOM_{{VARIANT}}.csv") {
}

BomOutputJob::BomOutputJob(const BomOutputJob& other) noexcept
  : OutputJob(other),
    mCustomAttributes(other.mCustomAttributes),
    mBoards(other.mBoards),
    mAssemblyVariants(other.mAssemblyVariants),
    mOutputPath(other.mOutputPath) {
}

BomOutputJob::BomOutputJob(const SExpression& node)
  : OutputJob(node),
    mCustomAttributes(),
    mBoards(node, "board"),
    mAssemblyVariants(node, "variant"),
    mOutputPath(node.getChild("output/@0").getValue()) {
  foreach (const SExpression* child, node.getChildren("custom_attribute")) {
    mCustomAttributes.append(child->getChild("@0").getValue());
  }
}

BomOutputJob::~BomOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString BomOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon BomOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/generate_bom.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BomOutputJob::setCustomAttributes(const QStringList& attrs) noexcept {
  if (attrs != mCustomAttributes) {
    mCustomAttributes = attrs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void BomOutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void BomOutputJob::setAssemblyVariants(const AssemblyVariantSet& avs) noexcept {
  if (avs != mAssemblyVariants) {
    mAssemblyVariants = avs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void BomOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> BomOutputJob::cloneShared() const noexcept {
  return std::make_shared<BomOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BomOutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  foreach (const QString& attribute, mCustomAttributes) {
    root.appendChild("custom_attribute", attribute);
    root.ensureLineBreak();
  }
  mBoards.serialize(root, "board");
  root.ensureLineBreak();
  mAssemblyVariants.serialize(root, "variant");
  root.ensureLineBreak();
  root.appendChild("output", mOutputPath);
}

bool BomOutputJob::equals(const OutputJob& rhs) const noexcept {
  const BomOutputJob& other = static_cast<const BomOutputJob&>(rhs);
  if (mCustomAttributes != other.mCustomAttributes) return false;
  if (mBoards != other.mBoards) return false;
  if (mAssemblyVariants != other.mAssemblyVariants) return false;
  if (mOutputPath != other.mOutputPath) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
