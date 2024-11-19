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
#include "packagecheckmessages.h"

#include "../../geometry/stroketext.h"
#include "../../types/layer.h"
#include "footprint.h"
#include "packagepad.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  MsgDeprecatedAssemblyType
 ******************************************************************************/

MsgDeprecatedAssemblyType::MsgDeprecatedAssemblyType() noexcept
  : RuleCheckMessage(Severity::Hint, tr("Non-recommended assembly type"),
                     tr("The assembly type 'Auto-detect' is not recommended as "
                        "the detection might not be correct in every case. "
                        "It's safer to specify the assembly type manually."),
                     "auto_assembly_type") {
}

/*******************************************************************************
 *  MsgSuspiciousAssemblyType
 ******************************************************************************/

MsgSuspiciousAssemblyType::MsgSuspiciousAssemblyType() noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Suspicious assembly type"),
        tr("The specified assembly type differs from the assembly type which "
           "is auto-detected from the footprint contents. Double-check if the "
           "specified assembly type is really correct."),
        "suspicious_assembly_type") {
}

/*******************************************************************************
 *  MsgDuplicatePadName
 ******************************************************************************/

MsgDuplicatePadName::MsgDuplicatePadName(const PackagePad& pad) noexcept
  : RuleCheckMessage(
        Severity::Error, tr("Duplicate pad name: '%1'").arg(*pad.getName()),
        tr("All package pads must have unique names, otherwise they cannot be "
           "distinguished later in the device editor. If your part has several "
           "leads with same functionality (e.g. multiple GND leads), you can "
           "assign all these pads to the same component signal later in the "
           "device editor.\n\nFor neutral packages (e.g. SOT23), pads should "
           "be named only by numbers anyway, not by functionality (e.g. name "
           "them '1', '2', '3' instead of 'D', 'G', 'S')."),
        "duplicate_pad_name") {
  mApproval->appendChild("name", *pad.getName());
}

/*******************************************************************************
 *  Class MsgFiducialClearanceLessThanStopMask
 ******************************************************************************/

MsgFiducialClearanceLessThanStopMask::MsgFiducialClearanceLessThanStopMask(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Small copper clearance on fiducial in '%1'")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("The copper clearance of the fiducial pad is less than its stop "
           "mask expansion, which is unusual. Typically the copper clearance "
           "should be equal to or greater than the stop mask expansion to "
           "avoid copper located within the stop mask opening."),
        "fiducial_copper_clearance_less_than_stop_mask"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  Class MsgFiducialStopMaskNotSet
 ******************************************************************************/

MsgFiducialStopMaskNotSet::MsgFiducialStopMaskNotSet(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Stop mask not set on fiducial in '%1'")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("The stop mask expansion of the fiducial pad is set to automatic, "
           "which is unusual. Typically the stop mask expansion of fiducials "
           "need to be manually set to a much larger value."),
        "fiducial_stop_mask_not_set"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgHoleWithoutStopMask
 ******************************************************************************/

MsgHoleWithoutStopMask::MsgHoleWithoutStopMask(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const Hole> hole) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("No stop mask on %1 hole in '%2'",
           "First placeholder is the hole diameter.")
            .arg(hole->getDiameter()->toMmString() % "mm",
                 *footprint->getNames().getDefaultValue()),
        tr("Non-plated holes should have a stop mask opening to avoid solder "
           "resist flowing into the hole. An automatic stop mask opening can "
           "be enabled in the hole properties."),
        "hole_without_stop_mask"),
    mFootprint(footprint),
    mHole(hole) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgInvalidCustomPadOutline
 ******************************************************************************/

MsgInvalidCustomPadOutline::MsgInvalidCustomPadOutline(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Invalid custom outline of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The pad has set a custom outline which does not represent a valid "
           "area. Either choose a different pad shape or specify a valid "
           "custom outline."),
        "invalid_custom_pad_outline"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgInvalidPadConnection
 ******************************************************************************/

MsgInvalidPadConnection::MsgInvalidPadConnection(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Invalid pad connection in '%1'")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("A footprint pad is connected to a package pad which doesn't exist. "
           "Check all pads for proper connections."),
        "invalid_pad_connection"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgMinimumWidthViolation
 ******************************************************************************/

MsgMinimumWidthViolation::MsgMinimumWidthViolation(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const Polygon> polygon, const Length& minWidth) noexcept
  : RuleCheckMessage(
        Severity::Warning, getMessage(footprint, polygon->getLayer()),
        tr("It is recommended that polygons on layer '%1' have a line width of "
           "at least %2.")
                .arg(polygon->getLayer().getNameTr())
                .arg(QString::number(minWidth.toMm() * 1000) % "μm") %
            " " % getDescriptionAppendix(),
        "thin_line"),
    mFootprint(footprint),
    mPolygon(polygon) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon->getUuid());
  mApproval->ensureLineBreak();
}

MsgMinimumWidthViolation::MsgMinimumWidthViolation(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const Circle> circle, const Length& minWidth) noexcept
  : RuleCheckMessage(
        Severity::Warning, getMessage(footprint, circle->getLayer()),
        tr("It is recommended that circles on layer '%1' have a line width of "
           "at least %2.")
                .arg(circle->getLayer().getNameTr())
                .arg(QString::number(minWidth.toMm() * 1000) % "μm") %
            " " % getDescriptionAppendix(),
        "thin_line"),
    mFootprint(footprint),
    mCircle(circle) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle->getUuid());
  mApproval->ensureLineBreak();
}

MsgMinimumWidthViolation::MsgMinimumWidthViolation(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const StrokeText> text, const Length& minWidth) noexcept
  : RuleCheckMessage(
        Severity::Warning, getMessage(footprint, text->getLayer()),
        tr("It is recommended that stroke texts on layer '%1' have a stroke "
           "width of at least %2.")
                .arg(text->getLayer().getNameTr())
                .arg(QString::number(minWidth.toMm() * 1000) % "μm") %
            " " % getDescriptionAppendix(),
        "thin_line"),
    mFootprint(footprint),
    mStrokeText(text) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("text", text->getUuid());
  mApproval->ensureLineBreak();
}

QString MsgMinimumWidthViolation::getMessage(
    std::shared_ptr<const Footprint> footprint, const Layer& layer) noexcept {
  return tr("Minimum width of '%1' in '%2'")
      .arg(layer.getNameTr())
      .arg(*footprint->getNames().getDefaultValue());
}

QString MsgMinimumWidthViolation::getDescriptionAppendix() noexcept {
  return tr(
      "Otherwise it could lead to manufacturing problems in some cases "
      "(depending on board settings and/or the capabilities of the PCB "
      "manufacturer).");
}

/*******************************************************************************
 *  MsgMissingCourtyard
 ******************************************************************************/

MsgMissingCourtyard::MsgMissingCourtyard(
    std::shared_ptr<const Footprint> footprint) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Missing courtyard in footprint '%1'")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("It is recommended to draw the package courtyard with a "
           "single, closed, zero-width polygon or circle on "
           "layer '%1'. This allows the DRC to warn if another device "
           "is placed within the courtyard of this device (i.e. too close).")
                .arg(Layer::topCourtyard().getNameTr()) %
            "\n\n" %
            tr("Often this is identical to the package outline but with a "
               "small offset. If you're unsure, just ignore this message."),
        "missing_courtyard"),
    mFootprint(footprint) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgMissingFootprint
 ******************************************************************************/

MsgMissingFootprint::MsgMissingFootprint() noexcept
  : RuleCheckMessage(
        Severity::Error, tr("No footprint defined"),
        tr("Every package must have at least one footprint, otherwise it can't "
           "be added to a board."),
        "missing_footprint") {
}

/*******************************************************************************
 *  MsgMissingFootprintModel
 ******************************************************************************/

MsgMissingFootprintModel::MsgMissingFootprintModel(
    std::shared_ptr<const Footprint> footprint) noexcept
  : RuleCheckMessage(
        Severity::Hint,
        tr("No 3D model defined for '%1'")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("The footprint has no 3D model specified, so the package will be "
           "missing in the 3D viewer and in 3D data exports. However, this has "
           "no impact on the PCB production data."),
        "missing_footprint_3d_model") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgMissingFootprintName
 ******************************************************************************/

MsgMissingFootprintName::MsgMissingFootprintName(
    std::shared_ptr<const Footprint> footprint) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Missing text '%1' in footprint '%2'")
            .arg("{{NAME}}", *footprint->getNames().getDefaultValue()),
        tr("Most footprints should have a text element for the component's "
           "name, otherwise you won't see that name on the PCB (e.g. on "
           "silkscreen). There are only a few exceptions which don't need a "
           "name (e.g. if the footprint is only a drawing), for those you can "
           "ignore this message."),
        "missing_name_text"),
    mFootprint(footprint) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgMissingFootprintValue
 ******************************************************************************/

MsgMissingFootprintValue::MsgMissingFootprintValue(
    std::shared_ptr<const Footprint> footprint) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Missing text '%1' in footprint '%2'")
            .arg("{{VALUE}}", *footprint->getNames().getDefaultValue()),
        tr("Most footprints should have a text element for the component's "
           "value, otherwise you won't see that value on the PCB (e.g. on "
           "silkscreen). There are only a few exceptions which don't need a "
           "value (e.g. if the footprint is only a drawing), for those you can "
           "ignore this message."),
        "missing_value_text"),
    mFootprint(footprint) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgMissingPackageOutline
 ******************************************************************************/

MsgMissingPackageOutline::MsgMissingPackageOutline(
    std::shared_ptr<const Footprint> footprint) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Missing outline in footprint '%1'")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("It is recommended to draw the package outline with a "
           "single, closed, zero-width polygon or circle on "
           "layer '%1'. This allows the DRC to warn if this device "
           "is placed within the courtyard of another device (i.e. too close).")
            .arg(Layer::topPackageOutlines().getNameTr()),
        "missing_package_outline"),
    mFootprint(footprint) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgFootprintOriginNotInCenter
 ******************************************************************************/

MsgFootprintOriginNotInCenter::MsgFootprintOriginNotInCenter(
    std::shared_ptr<const Footprint> footprint, const Point& center) noexcept
  : RuleCheckMessage(
        Severity::Hint,
        tr("Origin of '%1' not in center")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("Generally the origin (0, 0) should be at the coordinate used for "
           "pick&place which is typically in the center of the package body. "
           "It should even be (more or less) <b>exactly</b> in the center, not "
           "aligned to a grid (off-grid pads are fine).\n\nIt looks like this "
           "rule is not followed in this footprint. However, for irregular "
           "package shapes or other special cases this warning may not be "
           "justified. In such cases, just approve it."),
        "origin_not_in_center"),
    mFootprint(footprint),
    mCenter(center) {
}

/*******************************************************************************
 *  MsgOverlappingPads
 ******************************************************************************/

MsgOverlappingPads::MsgOverlappingPads(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad1, const QString& pkgPad1Name,
    std::shared_ptr<const FootprintPad> pad2,
    const QString& pkgPad2Name) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Overlapping pads '%1' and '%2' in '%3'")
            .arg(pkgPad1Name, pkgPad2Name,
                 *footprint->getNames().getDefaultValue()),
        tr("The copper area of two pads overlap. This can lead to serious "
           "issues with the design rule check and probably leads to a short "
           "circuit in the board so this really needs to be fixed."),
        "overlapping_pads"),
    mFootprint(footprint),
    mPad1(pad1),
    mPad2(pad2) {
}

/*******************************************************************************
 *  MsgPadAnnularRingViolation
 ******************************************************************************/

MsgPadAnnularRingViolation::MsgPadAnnularRingViolation(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName,
    const Length& annularRing) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Annular ring of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("Pads should have at least %1 annular ring (copper around each pad "
           "hole). Note that this value is just a general recommendation, the "
           "exact value depends on the capabilities of the PCB manufacturer.")
            .arg(QString::number(annularRing.toMm() * 1000) % "μm"),
        "small_pad_annular_ring"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgPadClearanceViolation
 ******************************************************************************/

MsgPadClearanceViolation::MsgPadClearanceViolation(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad1, const QString& pkgPad1Name,
    std::shared_ptr<const FootprintPad> pad2, const QString& pkgPad2Name,
    const Length& clearance) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance of pad '%1' to pad '%2' in '%3'")
            .arg(pkgPad1Name, pkgPad2Name,
                 *footprint->getNames().getDefaultValue()),
        tr("Pads should have at least %1 clearance between each other. In some "
           "situations it might be needed to use smaller clearances but not "
           "all PCB manufacturers are able to reliably produce such small "
           "clearances, so usually this should be avoided.")
            .arg(QString::number(clearance.toMm() * 1000) % "μm"),
        "small_pad_clearance"),
    mFootprint(footprint),
    mPad1(pad1),
    mPad2(pad2) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", std::min(pad1->getUuid(), pad2->getUuid()));
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", std::max(pad1->getUuid(), pad2->getUuid()));
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgPadHoleOutsideCopper
 ******************************************************************************/

MsgPadHoleOutsideCopper::MsgPadHoleOutsideCopper(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Hole outside copper of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("All THT pad holes must be fully surrounded by copper, otherwise "
           "they could lead to serious issues during the design rule check or "
           "manufacturing process."),
        "pad_hole_outside_copper"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgPadOriginOutsideCopper
 ******************************************************************************/

MsgPadOriginOutsideCopper::MsgPadOriginOutsideCopper(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Invalid origin of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The origin of each pad must be located within its copper area, "
           "otherwise traces won't be connected properly.\n\n"
           "For THT pads, the origin must be located within a drill "
           "hole since on some layers the pad might only have a small annular "
           "ring instead of the full pad shape."),
        "pad_origin_outside_copper"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgPadOverlapsWithLegend
 ******************************************************************************/

MsgPadOverlapsWithLegend::MsgPadOverlapsWithLegend(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName,
    const Length& clearance) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance of pad '%1' in '%2' to legend")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("Pads should have at least %1 clearance to drawings on the "
           "legend because these drawings would be cropped during the "
           "Gerber export when used as silkscreen.")
            .arg(QString::number(clearance.toMm() * 1000) % "μm"),
        "pad_overlaps_legend"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgPadStopMaskOff
 ******************************************************************************/

MsgPadStopMaskOff::MsgPadStopMaskOff(std::shared_ptr<const Footprint> footprint,
                                     std::shared_ptr<const FootprintPad> pad,
                                     const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Solder resist on pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("There's no stop mask opening enabled on the pad, so the copper "
           "pad will be covered by solder resist and is thus not functional. "
           "This is very unusual, you should double-check if this is really "
           "what you want."),
        "pad_stop_mask_off"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  Class MsgPadWithCopperClearance
 ******************************************************************************/

MsgPadWithCopperClearance::MsgPadWithCopperClearance(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Hint,
        tr("Copper clearance >0 on pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("There is a custom copper clearance enabled on the pad, which is "
           "unusual for pads which do not represent a fiducial. Note that the "
           "clearance value from the board design rules is applied to all pads "
           "anyway, thus manual clearance values are usually not needed. If "
           "this pad is a fiducial, make sure to set its function to the "
           "corresponding value."),
        "pad_with_copper_clearance"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgSmtPadWithSolderPaste
 ******************************************************************************/

MsgSmtPadWithSolderPaste::MsgSmtPadWithSolderPaste(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Solder paste on SMT pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The SMT pad has solder paste enabled, but its function indicates "
           "that there's no lead to be soldered on it (e.g. a fiducial). "
           "Usually solder paste is not desired on such special pads which "
           "won't be soldered."),
        "smt_pad_with_solder_paste"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgSmtPadWithoutSolderPaste
 ******************************************************************************/

MsgSmtPadWithoutSolderPaste::MsgSmtPadWithoutSolderPaste(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("No solder paste on SMT pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The SMT pad has no solder paste enabled, which is unusual since "
           "without solder paste the pad cannot be reflow soldered. Only use "
           "this if there's no lead to be soldered on that pad, or if you have "
           "drawn a manual solder paste area."),
        "smt_pad_without_solder_paste"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  Class MsgSuspiciousPadFunction
 ******************************************************************************/

MsgSuspiciousPadFunction::MsgSuspiciousPadFunction(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Suspicious function of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The configured pad function does not match other properties of the "
           "pad and thus looks suspicious. Possible reasons:\n\n"
           " - Function is intended for THT pads but pad is SMT\n"
           " - Function is intended for SMT pads but pad is THT\n"
           " - Function is electrical but pad is not connected\n"
           " - Function is fiducial but pad is connected"),
        "suspicious_pad_function"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgThtPadWithSolderPaste
 ******************************************************************************/

MsgThtPadWithSolderPaste::MsgThtPadWithSolderPaste(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Solder paste on THT pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The THT pad has solder paste enabled, which is very unusual since "
           "through-hole components are usually not reflow soldered. Also the "
           "solder paste could flow into the pads hole, possibly causing "
           "troubles during THT assembly. Double-check if this is really what "
           "you want."),
        "tht_pad_with_solder_paste"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  Class MsgUnspecifiedPadFunction
 ******************************************************************************/

MsgUnspecifiedPadFunction::MsgUnspecifiedPadFunction(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Hint,
        tr("Unspecified function of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The function of the pad is not specified, which could lead to "
           "inaccurate or wrong data in exports (e.g. pick&place files). Also "
           "the automatic checks can detect more potential issues if the "
           "function is specified. Thus it's recommended to explicitly specify "
           "the function of each pad.") %
            "\n\n" %
            tr("However, the image data of a PCB is not affected by the pad "
               "function."),
        "pad_function_unspecified"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgUnusedCustomPadOutline
 ******************************************************************************/

MsgUnusedCustomPadOutline::MsgUnusedCustomPadOutline(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Unused custom outline of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The pad has set a custom outline but it isn't used as the shape. "
           "So it has no effect and should be removed to avoid confusion."),
        "unused_custom_pad_outline"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgUselessZone
 ******************************************************************************/

MsgUselessZone::MsgUselessZone(std::shared_ptr<const Footprint> footprint,
                               std::shared_ptr<const Zone> zone) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Useless keepout zone in '%2'")
            .arg(*footprint->getNames().getDefaultValue()),
        tr("The keepout zone has no layer or rule enabled so it has no effect. "
           "Either correct its properties or remove it from the footprint."),
        "useless_zone"),
    mFootprint(footprint),
    mZone(zone) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("zone", zone->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgWrongFootprintTextLayer
 ******************************************************************************/

MsgWrongFootprintTextLayer::MsgWrongFootprintTextLayer(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const StrokeText> text, const Layer& expectedLayer) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Layer of '%1' in '%2' is not '%3'")
            .arg(text->getText(), *footprint->getNames().getDefaultValue(),
                 expectedLayer.getNameTr()),
        tr("The text element '%1' should normally be on layer '%2'.")
            .arg(text->getText(), expectedLayer.getNameTr()),
        "unusual_text_layer"),
    mFootprint(footprint),
    mText(text),
    mExpectedLayer(&expectedLayer) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("footprint", footprint->getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("text", text->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
