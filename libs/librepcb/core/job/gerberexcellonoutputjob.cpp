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
#include "gerberexcellonoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

// Note: Must build a "default style" configuration for defaultStyle().
GerberExcellonOutputJob::GerberExcellonOutputJob() noexcept
  : OutputJob(getTypeName(), Uuid::createRandom(),
              elementNameFromTr("GerberExcellonOutputJob",
                                QT_TR_NOOP("Gerber/Excellon"))),
    mSuffixDrills("_DRILLS.drl"),
    mSuffixDrillsNpth("_DRILLS-NPTH.drl"),
    mSuffixDrillsPth("_DRILLS-PTH.drl"),
    mSuffixDrillsBlindBuried(
        "_DRILLS-PLATED-{{START_LAYER}}-{{END_LAYER}}.drl"),
    mSuffixOutlines("_OUTLINES.gbr"),
    mSuffixCopperTop("_COPPER-TOP.gbr"),
    mSuffixCopperInner("_COPPER-IN{{CU_LAYER}}.gbr"),
    mSuffixCopperBot("_COPPER-BOTTOM.gbr"),
    mSuffixSolderMaskTop("_SOLDERMASK-TOP.gbr"),
    mSuffixSolderMaskBot("_SOLDERMASK-BOTTOM.gbr"),
    mSuffixSilkscreenTop("_SILKSCREEN-TOP.gbr"),
    mSuffixSilkscreenBot("_SILKSCREEN-BOTTOM.gbr"),
    mSuffixSolderPasteTop("_SOLDERPASTE-TOP.gbr"),
    mSuffixSolderPasteBot("_SOLDERPASTE-BOTTOM.gbr"),
    mMergeDrillFiles(false),
    mUseG85SlotCommand(false),
    mEnableSolderPasteTop(true),
    mEnableSolderPasteBot(true),
    mBoards(BoardSet::onlyDefault()),
    mOutputPath("gerber/{{PROJECT}}_{{VERSION}}") {
}

GerberExcellonOutputJob::GerberExcellonOutputJob(
    const GerberExcellonOutputJob& other) noexcept
  : OutputJob(other),
    mSuffixDrills(other.mSuffixDrills),
    mSuffixDrillsNpth(other.mSuffixDrillsNpth),
    mSuffixDrillsPth(other.mSuffixDrillsPth),
    mSuffixDrillsBlindBuried(other.mSuffixDrillsBlindBuried),
    mSuffixOutlines(other.mSuffixOutlines),
    mSuffixCopperTop(other.mSuffixCopperTop),
    mSuffixCopperInner(other.mSuffixCopperInner),
    mSuffixCopperBot(other.mSuffixCopperBot),
    mSuffixSolderMaskTop(other.mSuffixSolderMaskTop),
    mSuffixSolderMaskBot(other.mSuffixSolderMaskBot),
    mSuffixSilkscreenTop(other.mSuffixSilkscreenTop),
    mSuffixSilkscreenBot(other.mSuffixSilkscreenBot),
    mSuffixSolderPasteTop(other.mSuffixSolderPasteTop),
    mSuffixSolderPasteBot(other.mSuffixSolderPasteBot),
    mMergeDrillFiles(other.mMergeDrillFiles),
    mUseG85SlotCommand(other.mUseG85SlotCommand),
    mEnableSolderPasteTop(other.mEnableSolderPasteTop),
    mEnableSolderPasteBot(other.mEnableSolderPasteBot),
    mBoards(other.mBoards),
    mOutputPath(other.mOutputPath) {
}

GerberExcellonOutputJob::GerberExcellonOutputJob(const SExpression& node)
  : OutputJob(node),
    mSuffixDrills(node.getChild("drills/suffix_merged/@0").getValue()),
    mSuffixDrillsNpth(node.getChild("drills/suffix_npth/@0").getValue()),
    mSuffixDrillsPth(node.getChild("drills/suffix_pth/@0").getValue()),
    mSuffixDrillsBlindBuried(
        node.getChild("drills/suffix_buried/@0").getValue()),
    mSuffixOutlines(node.getChild("outlines/suffix/@0").getValue()),
    mSuffixCopperTop(node.getChild("copper_top/suffix/@0").getValue()),
    mSuffixCopperInner(node.getChild("copper_inner/suffix/@0").getValue()),
    mSuffixCopperBot(node.getChild("copper_bot/suffix/@0").getValue()),
    mSuffixSolderMaskTop(node.getChild("soldermask_top/suffix/@0").getValue()),
    mSuffixSolderMaskBot(node.getChild("soldermask_bot/suffix/@0").getValue()),
    mSuffixSilkscreenTop(node.getChild("silkscreen_top/suffix/@0").getValue()),
    mSuffixSilkscreenBot(node.getChild("silkscreen_bot/suffix/@0").getValue()),
    mSuffixSolderPasteTop(
        node.getChild("solderpaste_top/suffix/@0").getValue()),
    mSuffixSolderPasteBot(
        node.getChild("solderpaste_bot/suffix/@0").getValue()),
    mMergeDrillFiles(deserialize<bool>(node.getChild("drills/merge/@0"))),
    mUseG85SlotCommand(deserialize<bool>(node.getChild("drills/g85_slots/@0"))),
    mEnableSolderPasteTop(
        deserialize<bool>(node.getChild("solderpaste_top/create/@0"))),
    mEnableSolderPasteBot(
        deserialize<bool>(node.getChild("solderpaste_bot/create/@0"))),
    mBoards(node, "board"),
    mOutputPath(node.getChild("output/@0").getValue()) {
}

GerberExcellonOutputJob::~GerberExcellonOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString GerberExcellonOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon GerberExcellonOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/export_gerber.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GerberExcellonOutputJob::setSuffixDrills(const QString& s) noexcept {
  if (s != mSuffixDrills) {
    mSuffixDrills = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixDrillsNpth(const QString& s) noexcept {
  if (s != mSuffixDrillsNpth) {
    mSuffixDrillsNpth = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixDrillsPth(const QString& s) noexcept {
  if (s != mSuffixDrillsPth) {
    mSuffixDrillsPth = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixDrillsBlindBuried(
    const QString& s) noexcept {
  if (s != mSuffixDrillsBlindBuried) {
    mSuffixDrillsBlindBuried = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixOutlines(const QString& s) noexcept {
  if (s != mSuffixOutlines) {
    mSuffixOutlines = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixCopperTop(const QString& s) noexcept {
  if (s != mSuffixCopperTop) {
    mSuffixCopperTop = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixCopperInner(const QString& s) noexcept {
  if (s != mSuffixCopperInner) {
    mSuffixCopperInner = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixCopperBot(const QString& s) noexcept {
  if (s != mSuffixCopperBot) {
    mSuffixCopperBot = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixSolderMaskTop(
    const QString& s) noexcept {
  if (s != mSuffixSolderMaskTop) {
    mSuffixSolderMaskTop = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixSolderMaskBot(
    const QString& s) noexcept {
  if (s != mSuffixSolderMaskBot) {
    mSuffixSolderMaskBot = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixSilkscreenTop(
    const QString& s) noexcept {
  if (s != mSuffixSilkscreenTop) {
    mSuffixSilkscreenTop = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixSilkscreenBot(
    const QString& s) noexcept {
  if (s != mSuffixSilkscreenBot) {
    mSuffixSilkscreenBot = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixSolderPasteTop(
    const QString& s) noexcept {
  if (s != mSuffixSolderPasteTop) {
    mSuffixSolderPasteTop = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setSuffixSolderPasteBot(
    const QString& s) noexcept {
  if (s != mSuffixSolderPasteBot) {
    mSuffixSolderPasteBot = s;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setMergeDrillFiles(bool m) noexcept {
  if (m != mMergeDrillFiles) {
    mMergeDrillFiles = m;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setUseG85SlotCommand(bool u) noexcept {
  if (u != mUseG85SlotCommand) {
    mUseG85SlotCommand = u;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setEnableSolderPasteTop(bool e) noexcept {
  if (e != mEnableSolderPasteTop) {
    mEnableSolderPasteTop = e;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setEnableSolderPasteBot(bool e) noexcept {
  if (e != mEnableSolderPasteBot) {
    mEnableSolderPasteBot = e;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GerberExcellonOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> GerberExcellonOutputJob::cloneShared()
    const noexcept {
  return std::make_shared<GerberExcellonOutputJob>(*this);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

std::shared_ptr<GerberExcellonOutputJob>
    GerberExcellonOutputJob::defaultStyle() noexcept {
  return std::shared_ptr<GerberExcellonOutputJob>(
      new GerberExcellonOutputJob());
}

std::shared_ptr<GerberExcellonOutputJob>
    GerberExcellonOutputJob::protelStyle() noexcept {
  std::shared_ptr<GerberExcellonOutputJob> obj(new GerberExcellonOutputJob());
  obj->setSuffixDrills(".drl");
  obj->setSuffixDrillsNpth("_NPTH.drl");
  obj->setSuffixDrillsPth("_PTH.drl");
  obj->setSuffixDrillsBlindBuried("_L{{START_NUMBER}}-L{{END_NUMBER}}.drl");
  obj->setSuffixOutlines(".gml");
  obj->setSuffixCopperTop(".gtl");
  obj->setSuffixCopperInner(".g{{CU_LAYER}}");
  obj->setSuffixCopperBot(".gbl");
  obj->setSuffixSolderMaskTop(".gts");
  obj->setSuffixSolderMaskBot(".gbs");
  obj->setSuffixSilkscreenTop(".gto");
  obj->setSuffixSilkscreenBot(".gbo");
  obj->setSuffixSolderPasteTop(".gtp");
  obj->setSuffixSolderPasteBot(".gbp");
  obj->setMergeDrillFiles(true);
  return obj;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GerberExcellonOutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  root.appendList("outlines").appendChild("suffix", mSuffixOutlines);
  root.ensureLineBreak();
  root.appendList("copper_top").appendChild("suffix", mSuffixCopperTop);
  root.ensureLineBreak();
  root.appendList("copper_inner").appendChild("suffix", mSuffixCopperInner);
  root.ensureLineBreak();
  root.appendList("copper_bot").appendChild("suffix", mSuffixCopperBot);
  root.ensureLineBreak();
  root.appendList("soldermask_top").appendChild("suffix", mSuffixSolderMaskTop);
  root.ensureLineBreak();
  root.appendList("soldermask_bot").appendChild("suffix", mSuffixSolderMaskBot);
  root.ensureLineBreak();
  root.appendList("silkscreen_top").appendChild("suffix", mSuffixSilkscreenTop);
  root.ensureLineBreak();
  root.appendList("silkscreen_bot").appendChild("suffix", mSuffixSilkscreenBot);
  root.ensureLineBreak();

  SExpression& solderPasteTop = root.appendList("solderpaste_top");
  solderPasteTop.appendChild("create", mEnableSolderPasteTop);
  solderPasteTop.appendChild("suffix", mSuffixSolderPasteTop);
  root.ensureLineBreak();

  SExpression& solderPasteBot = root.appendList("solderpaste_bot");
  solderPasteBot.appendChild("create", mEnableSolderPasteBot);
  solderPasteBot.appendChild("suffix", mSuffixSolderPasteBot);
  root.ensureLineBreak();

  SExpression& drills = root.appendList("drills");
  drills.appendChild("merge", mMergeDrillFiles);
  drills.ensureLineBreak();
  drills.appendChild("suffix_pth", mSuffixDrillsPth);
  drills.ensureLineBreak();
  drills.appendChild("suffix_npth", mSuffixDrillsNpth);
  drills.ensureLineBreak();
  drills.appendChild("suffix_merged", mSuffixDrills);
  drills.ensureLineBreak();
  drills.appendChild("suffix_buried", mSuffixDrillsBlindBuried);
  drills.ensureLineBreak();
  drills.appendChild("g85_slots", mUseG85SlotCommand);
  drills.ensureLineBreak();
  root.ensureLineBreak();

  mBoards.serialize(root, "board");
  root.ensureLineBreak();
  root.appendChild("output", mOutputPath);
}

bool GerberExcellonOutputJob::equals(const OutputJob& rhs) const noexcept {
  const GerberExcellonOutputJob& other =
      static_cast<const GerberExcellonOutputJob&>(rhs);
  if (mSuffixDrills != other.mSuffixDrills) return false;
  if (mSuffixDrillsNpth != other.mSuffixDrillsNpth) return false;
  if (mSuffixDrillsPth != other.mSuffixDrillsPth) return false;
  if (mSuffixDrillsBlindBuried != other.mSuffixDrillsBlindBuried) return false;
  if (mSuffixOutlines != other.mSuffixOutlines) return false;
  if (mSuffixCopperTop != other.mSuffixCopperTop) return false;
  if (mSuffixCopperInner != other.mSuffixCopperInner) return false;
  if (mSuffixCopperBot != other.mSuffixCopperBot) return false;
  if (mSuffixSolderMaskTop != other.mSuffixSolderMaskTop) return false;
  if (mSuffixSolderMaskBot != other.mSuffixSolderMaskBot) return false;
  if (mSuffixSilkscreenTop != other.mSuffixSilkscreenTop) return false;
  if (mSuffixSilkscreenBot != other.mSuffixSilkscreenBot) return false;
  if (mSuffixSolderPasteTop != other.mSuffixSolderPasteTop) return false;
  if (mSuffixSolderPasteBot != other.mSuffixSolderPasteBot) return false;
  if (mMergeDrillFiles != other.mMergeDrillFiles) return false;
  if (mUseG85SlotCommand != other.mUseG85SlotCommand) return false;
  if (mEnableSolderPasteTop != other.mEnableSolderPasteTop) return false;
  if (mEnableSolderPasteBot != other.mEnableSolderPasteBot) return false;
  if (mBoards != other.mBoards) return false;
  if (mOutputPath != other.mOutputPath) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
