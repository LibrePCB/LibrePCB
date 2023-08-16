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
#include "pickplaceoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PickPlaceOutputJob::PickPlaceOutputJob() noexcept
  : OutputJob(
        getTypeName(), Uuid::createRandom(),
        elementNameFromTr("PickPlaceOutputJob", QT_TR_NOOP("Pick&Place CSV"))),
    mTechnologies(Technology::All),
    mIncludeComment(true),
    mBoards(BoardSet::onlyDefault()),
    mAssemblyVariants(AssemblyVariantSet::all()),
    mCreateTop(true),
    mCreateBottom(true),
    mCreateBoth(false),
    mOutputPathTop("assembly/{{PROJECT}}_{{VERSION}}_PnP_{{VARIANT}}_TOP.csv"),
    mOutputPathBottom(
        "assembly/{{PROJECT}}_{{VERSION}}_PnP_{{VARIANT}}_BOT.csv"),
    mOutputPathBoth("assembly/{{PROJECT}}_{{VERSION}}_PnP_{{VARIANT}}.csv") {
}

PickPlaceOutputJob::PickPlaceOutputJob(const PickPlaceOutputJob& other) noexcept
  : OutputJob(other),
    mTechnologies(other.mTechnologies),
    mIncludeComment(other.mIncludeComment),
    mBoards(other.mBoards),
    mAssemblyVariants(other.mAssemblyVariants),
    mCreateTop(other.mCreateTop),
    mCreateBottom(other.mCreateBottom),
    mCreateBoth(other.mCreateBoth),
    mOutputPathTop(other.mOutputPathTop),
    mOutputPathBottom(other.mOutputPathBottom),
    mOutputPathBoth(other.mOutputPathBoth) {
}

PickPlaceOutputJob::PickPlaceOutputJob(const SExpression& node)
  : OutputJob(node),
    mTechnologies(0),
    mIncludeComment(deserialize<bool>(node.getChild("comment/@0"))),
    mBoards(node, "board"),
    mAssemblyVariants(node, "variant"),
    mCreateTop(deserialize<bool>(node.getChild("top/create/@0"))),
    mCreateBottom(deserialize<bool>(node.getChild("bottom/create/@0"))),
    mCreateBoth(deserialize<bool>(node.getChild("both/create/@0"))),
    mOutputPathTop(node.getChild("top/output/@0").getValue()),
    mOutputPathBottom(node.getChild("bottom/output/@0").getValue()),
    mOutputPathBoth(node.getChild("both/output/@0").getValue()) {
  mTechnologies.setFlag(Technology::Tht,
                        deserialize<bool>(node.getChild("tht/@0")));
  mTechnologies.setFlag(Technology::Smt,
                        deserialize<bool>(node.getChild("smt/@0")));
  mTechnologies.setFlag(Technology::Mixed,
                        deserialize<bool>(node.getChild("mixed/@0")));
  mTechnologies.setFlag(Technology::Fiducial,
                        deserialize<bool>(node.getChild("fiducial/@0")));
  mTechnologies.setFlag(Technology::Other,
                        deserialize<bool>(node.getChild("other/@0")));
}

PickPlaceOutputJob::~PickPlaceOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString PickPlaceOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon PickPlaceOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/export_pick_place_file.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PickPlaceOutputJob::setTechnologies(Technologies technologies) noexcept {
  if (technologies != mTechnologies) {
    mTechnologies = technologies;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setIncludeComment(bool include) noexcept {
  if (include != mIncludeComment) {
    mIncludeComment = include;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setAssemblyVariants(
    const AssemblyVariantSet& avs) noexcept {
  if (avs != mAssemblyVariants) {
    mAssemblyVariants = avs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setCreateTop(bool create) noexcept {
  if (create != mCreateTop) {
    mCreateTop = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setCreateBottom(bool create) noexcept {
  if (create != mCreateBottom) {
    mCreateBottom = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setCreateBoth(bool create) noexcept {
  if (create != mCreateBoth) {
    mCreateBoth = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setOutputPathTop(const QString& path) noexcept {
  if (path != mOutputPathTop) {
    mOutputPathTop = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setOutputPathBottom(const QString& path) noexcept {
  if (path != mOutputPathBottom) {
    mOutputPathBottom = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

void PickPlaceOutputJob::setOutputPathBoth(const QString& path) noexcept {
  if (path != mOutputPathBoth) {
    mOutputPathBoth = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> PickPlaceOutputJob::cloneShared() const noexcept {
  return std::make_shared<PickPlaceOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PickPlaceOutputJob::serializeDerived(SExpression& root) const {
  root.appendChild("comment", mIncludeComment);
  root.ensureLineBreak();
  root.appendChild("tht", mTechnologies.testFlag(Technology::Tht));
  root.appendChild("smt", mTechnologies.testFlag(Technology::Smt));
  root.appendChild("mixed", mTechnologies.testFlag(Technology::Mixed));
  root.appendChild("fiducial", mTechnologies.testFlag(Technology::Fiducial));
  root.appendChild("other", mTechnologies.testFlag(Technology::Other));
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
  root.ensureLineBreak();
  SExpression& both = root.appendList("both");
  both.appendChild("create", mCreateBoth);
  both.appendChild("output", mOutputPathBoth);
}

bool PickPlaceOutputJob::equals(const OutputJob& rhs) const noexcept {
  const PickPlaceOutputJob& other = static_cast<const PickPlaceOutputJob&>(rhs);
  if (mTechnologies != other.mTechnologies) return false;
  if (mIncludeComment != other.mIncludeComment) return false;
  if (mBoards != other.mBoards) return false;
  if (mAssemblyVariants != other.mAssemblyVariants) return false;
  if (mCreateTop != other.mCreateTop) return false;
  if (mCreateBottom != other.mCreateBottom) return false;
  if (mCreateBoth != other.mCreateBoth) return false;
  if (mOutputPathTop != other.mOutputPathTop) return false;
  if (mOutputPathBottom != other.mOutputPathBottom) return false;
  if (mOutputPathBoth != other.mOutputPathBoth) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
