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
#include "gerberx3outputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GerberX3OutputJob::GerberX3OutputJob() noexcept
  : OutputJob(
        getTypeName(), Uuid::createRandom(),
        elementNameFromTr("GerberX3OutputJob", QT_TR_NOOP("Pick&Place X3"))),
    mBoards(BoardSet::onlyDefault()),
    mAssemblyVariants(AssemblyVariantSet::all()),
    mCreateTop(true),
    mCreateBottom(true),
    mOutputPathTop("assembly/{{PROJECT}}_{{VERSION}}_PnP_{{VARIANT}}_TOP.gbr"),
    mOutputPathBottom(
        "assembly/{{PROJECT}}_{{VERSION}}_PnP_{{VARIANT}}_BOT.gbr") {
}

GerberX3OutputJob::GerberX3OutputJob(const GerberX3OutputJob& other) noexcept
  : OutputJob(other),
    mBoards(other.mBoards),
    mAssemblyVariants(other.mAssemblyVariants),
    mCreateTop(other.mCreateTop),
    mCreateBottom(other.mCreateBottom),
    mOutputPathTop(other.mOutputPathTop),
    mOutputPathBottom(other.mOutputPathBottom) {
}

GerberX3OutputJob::GerberX3OutputJob(const SExpression& node)
  : OutputJob(node),
    mBoards(node, "board"),
    mAssemblyVariants(node, "variant"),
    mCreateTop(deserialize<bool>(node.getChild("top/create/@0"))),
    mCreateBottom(deserialize<bool>(node.getChild("bottom/create/@0"))),
    mOutputPathTop(node.getChild("top/output/@0").getValue()),
    mOutputPathBottom(node.getChild("bottom/output/@0").getValue()) {
}

GerberX3OutputJob::~GerberX3OutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString GerberX3OutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon GerberX3OutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/export_pick_place_file.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GerberX3OutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setAssemblyVariants(
    const AssemblyVariantSet& avs) noexcept {
  if (avs != mAssemblyVariants) {
    mAssemblyVariants = avs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setCreateTop(bool create) noexcept {
  if (create != mCreateTop) {
    mCreateTop = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setCreateBottom(bool create) noexcept {
  if (create != mCreateBottom) {
    mCreateBottom = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setOutputPathTop(const QString& path) noexcept {
  if (path != mOutputPathTop) {
    mOutputPathTop = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setOutputPathBottom(const QString& path) noexcept {
  if (path != mOutputPathBottom) {
    mOutputPathBottom = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> GerberX3OutputJob::cloneShared() const noexcept {
  return std::make_shared<GerberX3OutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GerberX3OutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  mBoards.serialize(root, "board");
  root.ensureLineBreak();
  mAssemblyVariants.serialize(root, "variant");
  root.ensureLineBreak();
  SExpression& top = root.appendList("top");
  top.appendChild("create", mCreateTop);
  top.appendChild("output", mOutputPathTop);
  root.ensureLineBreak();
  SExpression& bottom = root.appendList("bottom");
  bottom.appendChild("create", mCreateBottom);
  bottom.appendChild("output", mOutputPathBottom);
}

bool GerberX3OutputJob::equals(const OutputJob& rhs) const noexcept {
  const GerberX3OutputJob& other = static_cast<const GerberX3OutputJob&>(rhs);
  if (mBoards != other.mBoards) return false;
  if (mAssemblyVariants != other.mAssemblyVariants) return false;
  if (mCreateTop != other.mCreateTop) return false;
  if (mCreateBottom != other.mCreateBottom) return false;
  if (mOutputPathTop != other.mOutputPathTop) return false;
  if (mOutputPathBottom != other.mOutputPathBottom) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
