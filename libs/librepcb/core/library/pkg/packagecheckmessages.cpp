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
#include "../../graphics/graphicslayer.h"
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
  mApproval.appendChild("name", *pad.getName());
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("hole", hole->getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
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
        "missing_name_text") {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
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
        "missing_value_text") {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", std::min(pad1->getUuid(), pad2->getUuid()));
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", std::max(pad1->getUuid(), pad2->getUuid()));
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  MsgPadOverlapsWithPlacement
 ******************************************************************************/

MsgPadOverlapsWithPlacement::MsgPadOverlapsWithPlacement(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName,
    const Length& clearance) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance of pad '%1' in '%2' to placement layer")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("Pads should have at least %1 clearance to the outlines "
           "layer because outlines are drawn on silkscreen which will "
           "be cropped for Gerber export.")
            .arg(QString::number(clearance.toMm() * 1000) % "μm"),
        "pad_overlaps_placement"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  MsgWrongFootprintTextLayer
 ******************************************************************************/

MsgWrongFootprintTextLayer::MsgWrongFootprintTextLayer(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const StrokeText> text,
    const QString& expectedLayerName) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Layer of '%1' in '%2' is not '%3'")
            .arg(text->getText(), *footprint->getNames().getDefaultValue(),
                 GraphicsLayer::getTranslation(expectedLayerName)),
        tr("The text element '%1' should normally be on layer '%2'.")
            .arg(text->getText(),
                 GraphicsLayer::getTranslation(expectedLayerName)),
        "unusual_text_layer"),
    mFootprint(footprint),
    mText(text),
    mExpectedLayerName(expectedLayerName) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("text", text->getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
