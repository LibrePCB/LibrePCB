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
#include "interactivehtmlbomoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InteractiveHtmlBomOutputJob::InteractiveHtmlBomOutputJob() noexcept
  : OutputJob(getTypeName(), Uuid::createRandom(),
              elementNameFromTr("InteractiveHtmlBomOutputJob",
                                QT_TR_NOOP("Interactive Bill of Materials"))),
    mViewMode(InteractiveHtmlBom::ViewMode::LeftRight),
    mHighlightPin1(InteractiveHtmlBom::HighlightPin1Mode::None),
    mDarkMode(false),
    mBoardRotation(Angle::deg0()),
    mOffsetBackRotation(false),
    mShowSilkscreen(true),
    mShowFabrication(true),
    mShowPads(true),
    mShowTracks(true),
    mShowZones(true),
    mCheckBoxes{"Sourced", "Placed"},
    mComponentOrder{"C", "R", "L", "D", "U", "Y", "X", "F"},
    mCustomAttributes(),
    mBoards(BoardSet::onlyDefault()),
    mAssemblyVariants(AssemblyVariantSet::all()),
    mOutputPath("assembly/{{PROJECT}}_{{VERSION}}_BOM_{{VARIANT}}.html") {
}

InteractiveHtmlBomOutputJob::InteractiveHtmlBomOutputJob(
    const InteractiveHtmlBomOutputJob& other) noexcept
  : OutputJob(other),
    mViewMode(other.mViewMode),
    mHighlightPin1(other.mHighlightPin1),
    mDarkMode(other.mDarkMode),
    mBoardRotation(other.mBoardRotation),
    mOffsetBackRotation(other.mOffsetBackRotation),
    mShowSilkscreen(other.mShowSilkscreen),
    mShowFabrication(other.mShowFabrication),
    mShowPads(other.mShowPads),
    mShowTracks(other.mShowTracks),
    mShowZones(other.mShowZones),
    mCheckBoxes(other.mCheckBoxes),
    mComponentOrder(other.mComponentOrder),
    mCustomAttributes(other.mCustomAttributes),
    mBoards(other.mBoards),
    mAssemblyVariants(other.mAssemblyVariants),
    mOutputPath(other.mOutputPath) {
}

InteractiveHtmlBomOutputJob::InteractiveHtmlBomOutputJob(
    const SExpression& node)
  : OutputJob(node),
    mViewMode(deserialize<InteractiveHtmlBom::ViewMode>(
        node.getChild("view_mode/@0"))),
    mHighlightPin1(deserialize<InteractiveHtmlBom::HighlightPin1Mode>(
        node.getChild("highlight_pin1/@0"))),
    mDarkMode(deserialize<bool>(node.getChild("dark_mode/@0"))),
    mBoardRotation(deserialize<Angle>(node.getChild("rotation/@0"))),
    mOffsetBackRotation(
        deserialize<bool>(node.getChild("offset_back_rotation/@0"))),
    mShowSilkscreen(deserialize<bool>(node.getChild("show_silkscreen/@0"))),
    mShowFabrication(deserialize<bool>(node.getChild("show_fabrication/@0"))),
    mShowPads(deserialize<bool>(node.getChild("show_pads/@0"))),
    mShowTracks(deserialize<bool>(node.getChild("show_tracks/@0"))),
    mShowZones(deserialize<bool>(node.getChild("show_zones/@0"))),
    mCheckBoxes(node.getChild("checkboxes/@0").getValue().split(",")),
    mComponentOrder(node.getChild("component_order/@0").getValue().split(",")),
    mCustomAttributes(),
    mBoards(node, "board"),
    mAssemblyVariants(node, "variant"),
    mOutputPath(node.getChild("output/@0").getValue()) {
  foreach (const SExpression* child, node.getChildren("custom_attribute")) {
    mCustomAttributes.append(child->getChild("@0").getValue());
  }
}

InteractiveHtmlBomOutputJob::~InteractiveHtmlBomOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString InteractiveHtmlBomOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon InteractiveHtmlBomOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/generate_ibom.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void InteractiveHtmlBomOutputJob::setViewMode(
    InteractiveHtmlBom::ViewMode mode) noexcept {
  if (mode != mViewMode) {
    mViewMode = mode;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setDarkMode(bool dark) noexcept {
  if (dark != mDarkMode) {
    mDarkMode = dark;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setBoardRotation(const Angle& rot) noexcept {
  if (rot != mBoardRotation) {
    mBoardRotation = rot;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setOffsetBackRotation(bool offset) noexcept {
  if (offset != mOffsetBackRotation) {
    mOffsetBackRotation = offset;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setHighlightPin1(
    InteractiveHtmlBom::HighlightPin1Mode mode) noexcept {
  if (mode != mHighlightPin1) {
    mHighlightPin1 = mode;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setShowSilkscreen(bool show) noexcept {
  if (show != mShowSilkscreen) {
    mShowSilkscreen = show;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setShowFabrication(bool show) noexcept {
  if (show != mShowFabrication) {
    mShowFabrication = show;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setShowPads(bool show) noexcept {
  if (show != mShowPads) {
    mShowPads = show;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setShowTracks(bool show) noexcept {
  if (show != mShowTracks) {
    mShowTracks = show;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setShowZones(bool show) noexcept {
  if (show != mShowZones) {
    mShowZones = show;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setCheckBoxes(
    const QStringList& cbx) noexcept {
  if (cbx != mCheckBoxes) {
    mCheckBoxes = cbx;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setComponentOrder(
    const QStringList& order) noexcept {
  if (order != mComponentOrder) {
    mComponentOrder = order;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setCustomAttributes(
    const QStringList& attrs) noexcept {
  if (attrs != mCustomAttributes) {
    mCustomAttributes = attrs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setBoards(const BoardSet& boards) noexcept {
  if (boards != mBoards) {
    mBoards = boards;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setAssemblyVariants(
    const AssemblyVariantSet& avs) noexcept {
  if (avs != mAssemblyVariants) {
    mAssemblyVariants = avs;
    onEdited.notify(Event::PropertyChanged);
  }
}

void InteractiveHtmlBomOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> InteractiveHtmlBomOutputJob::cloneShared()
    const noexcept {
  return std::make_shared<InteractiveHtmlBomOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void InteractiveHtmlBomOutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  root.appendChild("view_mode", mViewMode);
  root.appendChild("highlight_pin1", mHighlightPin1);
  root.appendChild("dark_mode", mDarkMode);
  root.ensureLineBreak();
  root.appendChild("rotation", mBoardRotation);
  root.appendChild("offset_back_rotation", mOffsetBackRotation);
  root.ensureLineBreak();
  root.appendChild("show_silkscreen", mShowSilkscreen);
  root.appendChild("show_fabrication", mShowFabrication);
  root.ensureLineBreak();
  root.appendChild("show_pads", mShowPads);
  root.appendChild("show_tracks", mShowTracks);
  root.appendChild("show_zones", mShowZones);
  root.ensureLineBreak();
  root.appendChild("checkboxes", mCheckBoxes.join(","));
  root.ensureLineBreak();
  root.appendChild("component_order", mComponentOrder.join(","));
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

bool InteractiveHtmlBomOutputJob::equals(const OutputJob& rhs) const noexcept {
  const InteractiveHtmlBomOutputJob& other =
      static_cast<const InteractiveHtmlBomOutputJob&>(rhs);
  if (mViewMode != other.mViewMode) return false;
  if (mHighlightPin1 != other.mHighlightPin1) return false;
  if (mDarkMode != other.mDarkMode) return false;
  if (mBoardRotation != other.mBoardRotation) return false;
  if (mOffsetBackRotation != other.mOffsetBackRotation) return false;
  if (mShowSilkscreen != other.mShowSilkscreen) return false;
  if (mShowFabrication != other.mShowFabrication) return false;
  if (mShowPads != other.mShowPads) return false;
  if (mShowTracks != other.mShowTracks) return false;
  if (mShowZones != other.mShowZones) return false;
  if (mCheckBoxes != other.mCheckBoxes) return false;
  if (mComponentOrder != other.mComponentOrder) return false;
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
