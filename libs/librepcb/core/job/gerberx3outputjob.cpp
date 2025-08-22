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
  : OutputJob(getTypeName(), Uuid::createRandom(),
              elementNameFromTr("GerberX3OutputJob",
                                QT_TR_NOOP("Pick&Place / Glue Mask"))),
    mBoards(BoardSet::onlyDefault()),
    mAssemblyVariants(AssemblyVariantSet::all()),
    mEnableComponentsTop(true),
    mEnableComponentsBot(true),
    mOutputPathComponentsTop(
        "assembly/{{PROJECT}}_{{VERSION}}_PnP_{{VARIANT}}_TOP.gbr"),
    mOutputPathComponentsBot(
        "assembly/{{PROJECT}}_{{VERSION}}_PnP_{{VARIANT}}_BOT.gbr"),
    mEnableGlueTop(false),
    mEnableGlueBot(false),
    mOutputPathGlueTop(
        "assembly/{{PROJECT}}_{{VERSION}}_GLUE_{{VARIANT}}_TOP.gbr"),
    mOutputPathGlueBot(
        "assembly/{{PROJECT}}_{{VERSION}}_GLUE_{{VARIANT}}_BOT.gbr") {
}

GerberX3OutputJob::GerberX3OutputJob(const GerberX3OutputJob& other) noexcept
  : OutputJob(other),
    mBoards(other.mBoards),
    mAssemblyVariants(other.mAssemblyVariants),
    mEnableComponentsTop(other.mEnableComponentsTop),
    mEnableComponentsBot(other.mEnableComponentsBot),
    mOutputPathComponentsTop(other.mOutputPathComponentsTop),
    mOutputPathComponentsBot(other.mOutputPathComponentsBot),
    mEnableGlueTop(other.mEnableGlueTop),
    mEnableGlueBot(other.mEnableGlueBot),
    mOutputPathGlueTop(other.mOutputPathGlueTop),
    mOutputPathGlueBot(other.mOutputPathGlueBot) {
}

GerberX3OutputJob::GerberX3OutputJob(const SExpression& node)
  : OutputJob(node),
    mBoards(node, "board"),
    mAssemblyVariants(node, "variant"),
    mEnableComponentsTop(
        deserialize<bool>(node.getChild("components_top/create/@0"))),
    mEnableComponentsBot(
        deserialize<bool>(node.getChild("components_bot/create/@0"))),
    mOutputPathComponentsTop(
        node.getChild("components_top/output/@0").getValue()),
    mOutputPathComponentsBot(
        node.getChild("components_bot/output/@0").getValue()),
    mEnableGlueTop(deserialize<bool>(node.getChild("glue_top/create/@0"))),
    mEnableGlueBot(deserialize<bool>(node.getChild("glue_bot/create/@0"))),
    mOutputPathGlueTop(node.getChild("glue_top/output/@0").getValue()),
    mOutputPathGlueBot(node.getChild("glue_bot/output/@0").getValue()) {
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
  if ((mEnableGlueTop || mEnableGlueBot) && (!mEnableComponentsTop) &&
      (!mEnableComponentsBot)) {
    return QIcon(":/img/glue.png");
  } else {
    return QIcon(":/img/actions/export_pick_place_file.png");
  }
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

void GerberX3OutputJob::setEnableComponentsTop(bool create) noexcept {
  if (create != mEnableComponentsTop) {
    mEnableComponentsTop = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setEnableComponentsBot(bool create) noexcept {
  if (create != mEnableComponentsBot) {
    mEnableComponentsBot = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setOutputPathComponentsTop(
    const QString& path) noexcept {
  if (path != mOutputPathComponentsTop) {
    mOutputPathComponentsTop = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setOutputPathComponentsBot(
    const QString& path) noexcept {
  if (path != mOutputPathComponentsBot) {
    mOutputPathComponentsBot = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setEnableGlueTop(bool create) noexcept {
  if (create != mEnableGlueTop) {
    mEnableGlueTop = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setEnableGlueBot(bool create) noexcept {
  if (create != mEnableGlueBot) {
    mEnableGlueBot = create;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setOutputPathGlueTop(const QString& path) noexcept {
  if (path != mOutputPathGlueTop) {
    mOutputPathGlueTop = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberX3OutputJob::setOutputPathGlueBot(const QString& path) noexcept {
  if (path != mOutputPathGlueBot) {
    mOutputPathGlueBot = path;
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
  SExpression& cmpTop = root.appendList("components_top");
  cmpTop.appendChild("create", mEnableComponentsTop);
  cmpTop.appendChild("output", mOutputPathComponentsTop);
  root.ensureLineBreak();
  SExpression& cmpBot = root.appendList("components_bot");
  cmpBot.appendChild("create", mEnableComponentsBot);
  cmpBot.appendChild("output", mOutputPathComponentsBot);
  SExpression& glueTop = root.appendList("glue_top");
  glueTop.appendChild("create", mEnableGlueTop);
  glueTop.appendChild("output", mOutputPathGlueTop);
  root.ensureLineBreak();
  SExpression& glueBot = root.appendList("glue_bot");
  glueBot.appendChild("create", mEnableGlueBot);
  glueBot.appendChild("output", mOutputPathGlueBot);
  root.ensureLineBreak();
}

bool GerberX3OutputJob::equals(const OutputJob& rhs) const noexcept {
  const GerberX3OutputJob& other = static_cast<const GerberX3OutputJob&>(rhs);
  if (mBoards != other.mBoards) return false;
  if (mAssemblyVariants != other.mAssemblyVariants) return false;
  if (mEnableComponentsTop != other.mEnableComponentsTop) return false;
  if (mEnableComponentsBot != other.mEnableComponentsBot) return false;
  if (mOutputPathComponentsTop != other.mOutputPathComponentsTop) return false;
  if (mOutputPathComponentsBot != other.mOutputPathComponentsBot) return false;
  if (mEnableGlueTop != other.mEnableGlueTop) return false;
  if (mEnableGlueBot != other.mEnableGlueBot) return false;
  if (mOutputPathGlueTop != other.mOutputPathGlueTop) return false;
  if (mOutputPathGlueBot != other.mOutputPathGlueBot) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
