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
#include "boarddesignrulecheckmessages.h"

#include "../../../geometry/circle.h"
#include "../../../geometry/hole.h"
#include "../../../geometry/polygon.h"
#include "../../../library/pkg/packagepad.h"
#include "../../../utils/transform.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/netsignal.h"
#include "../board.h"
#include "../items/bi_device.h"
#include "../items/bi_footprintpad.h"
#include "../items/bi_hole.h"
#include "../items/bi_netline.h"
#include "../items/bi_netpoint.h"
#include "../items/bi_netsegment.h"
#include "../items/bi_plane.h"
#include "../items/bi_polygon.h"
#include "../items/bi_stroketext.h"
#include "../items/bi_via.h"
#include "../items/bi_zone.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

static QString seriousTroublesTr() {
  return QCoreApplication::translate(
      "BoardDesignRuleCheckMessages",
      "Depending on the capabilities of the PCB manufacturer, this could "
      "cause higher costs or even serious troubles during production, leading "
      "to a possibly non-functional PCB.");
}

static QString netNameWithFallback(const QString& netName) {
  if (!netName.isEmpty()) {
    return netName;
  } else {
    return QCoreApplication::translate("BoardDesignRuleCheckMessages",
                                       "(no net)");
  }
}

/*******************************************************************************
 *  DrcHoleRef
 ******************************************************************************/

void DrcHoleRef::serialize(SExpression& node) const {
  if (mDevice && mPad && mHole) {
    node.ensureLineBreak();
    node.appendChild("device", mDevice->uuid);
    node.ensureLineBreak();
    node.appendChild("pad", mPad->uuid);
    node.ensureLineBreak();
    node.appendChild("hole", mHole->uuid);
    node.ensureLineBreak();
  } else if (mDevice && mHole) {
    node.ensureLineBreak();
    node.appendChild("device", mDevice->uuid);
    node.ensureLineBreak();
    node.appendChild("hole", mHole->uuid);
    node.ensureLineBreak();
  } else if (mSegment && mVia) {
    node.ensureLineBreak();
    node.appendChild("netsegment", mSegment->uuid);
    node.ensureLineBreak();
    node.appendChild("via", mVia->uuid);
    node.ensureLineBreak();
  } else if (mHole) {
    node.appendChild("hole", mHole->uuid);
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown drill clearance object.");
  }
}

/*******************************************************************************
 *  DrcMsgMissingDevice
 ******************************************************************************/

DrcMsgMissingDevice::DrcMsgMissingDevice(const Uuid& uuid,
                                         const QString& name) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Missing device: '%1'", "Placeholders: Device name").arg(name),
        tr("There's a component in the schematics without a corresponding "
           "device in the board, so the circuit of the PCB is not "
           "complete.\n\nUse the \"Place Devices\" dock to add the device."),
        "missing_device", {}) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMissingConnection
 ******************************************************************************/

QString DrcMsgMissingConnection::Anchor::getName() const {
  QString name;
  if (mDevice && mPad) {
    name = "'" % mDevice->cmpInstanceName;
    if (!mPad->libPkgPadName.isEmpty()) {
      name += ":" % mPad->libPkgPadName;
    }
    name += "'";
  } else if (mSegment && mVia) {
    name = tr("Via");
  } else if (mSegment && mJunction) {
    name = tr("Trace");
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown airwire anchor type.");
  }
  return name;
}

void DrcMsgMissingConnection::Anchor::serialize(SExpression& node) const {
  if (mDevice && mPad) {
    node.ensureLineBreak();
    node.appendChild("device", mDevice->uuid);
    node.ensureLineBreak();
    node.appendChild("pad", mPad->uuid);
    node.ensureLineBreak();
  } else if (mSegment && mVia) {
    // I *guess* we don't need to serialize the via since only one connection
    // between a net segment any any other object can be missing(?).
    node.appendChild("netsegment", mSegment->uuid);
  } else if (mSegment && mJunction) {
    // Same as for vias.
    node.appendChild("netsegment", mSegment->uuid);
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown airwire anchor type.");
  }
}

DrcMsgMissingConnection::DrcMsgMissingConnection(const Anchor& p1,
                                                 const Anchor& p2,
                                                 const QString& netName,
                                                 const QVector<Path>& locations)
  : RuleCheckMessage(
        Severity::Error,
        tr("Missing connection in '%1': %2 ↔ %3",
           "Placeholders: Net name, connection count")
            .arg(netName, p1.getName(), p2.getName()),
        tr("There is a missing connection in the net, i.e. not all net items "
           "are connected together.\n\nAdd traces and/or planes to create the "
           "missing connections.\n\nNote that traces need to be snapped to the "
           "origin of footprint pads to make the airwire and this message "
           "disappearing."),
        "missing_connection", locations) {
  mApproval->ensureLineBreak();
  SExpression& fromNode = mApproval->appendList("tmp");
  mApproval->ensureLineBreak();
  SExpression& toNode = mApproval->appendList("tmp");
  mApproval->ensureLineBreak();

  // Sort nodes to make the approval canonical.
  p1.serialize(fromNode);
  p2.serialize(toNode);
  if (toNode < fromNode) {
    std::swap(fromNode, toNode);
  }
  fromNode.setName("from");
  toNode.setName("to");
}

/*******************************************************************************
 *  DrcMsgMissingBoardOutline
 ******************************************************************************/

DrcMsgMissingBoardOutline::DrcMsgMissingBoardOutline() noexcept
  : RuleCheckMessage(Severity::Error, tr("Missing board outline"),
                     tr("There's no board outline defined at all, so the board "
                        "cannot be manufactured.") %
                         "\n\n" %
                         tr("Add a closed, zero-width polygon on the layer "
                            "'%1' to draw the board outline.")
                             .arg(Layer::boardOutlines().getNameTr()),
                     "missing_board_outline", {}) {
}

/*******************************************************************************
 *  DrcMsgMultipleBoardOutlines
 ******************************************************************************/

DrcMsgMultipleBoardOutlines::DrcMsgMultipleBoardOutlines(
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Multiple board outlines"),
        tr("There are multiple, independent board outlines defined.") % "\n\n" %
            tr("Either add only a single board outline or make sure the PCB "
               "manufacturer can handle production data containing multiple "
               "PCBs."),
        "multiple_board_outlines", locations) {
}

/*******************************************************************************
 *  DrcMsgOpenBoardOutlinePolygon
 ******************************************************************************/

DrcMsgOpenBoardOutlinePolygon::DrcMsgOpenBoardOutlinePolygon(
    const Uuid& polygon, const std::optional<Uuid>& device,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error, tr("Non-closed board outline"),
        tr("The board outline polygon is not closed, i.e. the last vertex is "
           "not at the same coordinate as the first vertex.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Replace multiple coincident polygons with a single, connected "
               "polygon and append an explicit last vertex to make the polygon "
               "closed."),
        "open_board_outline", locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", *device);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("polygon", polygon);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumBoardOutlineInnerRadiusViolation
 ******************************************************************************/

DrcMsgMinimumBoardOutlineInnerRadiusViolation::
    DrcMsgMinimumBoardOutlineInnerRadiusViolation(
        const UnsignedLength& minRadius,
        const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Board outline inner radius < %1 %2",
           "Placeholders: Minimum radius, unit")
            .arg(minRadius->toMmString(), "mm"),
        tr("The board outline polygon is not manufacturable with the minimum "
           "tool diameter configured in the DRC settings due to edges with a "
           "smaller radius. Thus the actually produced board outline might "
           "contain larger edge radii and too small cutouts might even be "
           "missing completely.") %
            "\n\n" %
            tr("Check the DRC settings and add/increase the radius of inner "
               "board edges if needed."),
        "minimum_board_inner_radius_violation", locations) {
}

/*******************************************************************************
 *  DrcMsgEmptyNetSegment
 ******************************************************************************/

DrcMsgEmptyNetSegment::DrcMsgEmptyNetSegment(const Data::Segment& ns) noexcept
  : RuleCheckMessage(Severity::Hint,
                     tr("Empty segment of net '%1': '%2'",
                        "Placeholders: Net name, segment UUID")
                         .arg(netNameWithFallback(ns.netName), ns.uuid.toStr()),
                     "There's a net segment in the board without any via or "
                     "trace. This should not happen, please report it as a "
                     "bug. But no worries, this issue is not harmful at all "
                     "so you can safely ignore this message.",
                     "empty_netsegment", {}) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgUnconnectedJunction
 ******************************************************************************/

DrcMsgUnconnectedJunction::DrcMsgUnconnectedJunction(
    const Data::Junction& junction, const Data::Segment& ns,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Hint,
        tr("Unconnected junction in net: '%1'")
            .arg(netNameWithFallback(ns.netName)),
        "There's an invisible junction in the board without any trace "
        "attached. This should not happen, please report it as a bug. But "
        "no worries, this issue is not harmful at all so you can safely "
        "ignore this message.",
        "unconnected_junction", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("junction", junction.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumTextHeightViolation
 ******************************************************************************/

DrcMsgMinimumTextHeightViolation::DrcMsgMinimumTextHeightViolation(
    const Data::StrokeText& st, const BoardDesignRuleCheckData::Device* device,
    const UnsignedLength& minHeight, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Text height on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual height, minimum height, unit")
            .arg(st.layer->getNameTr(), st.height->toMmString(),
                 minHeight->toMmString(), "mm"),
        tr("The text height is smaller than the minimum height configured "
           "in the DRC settings. If the text is smaller than the minimum "
           "height specified by the PCB manufacturer, it may not be readable "
           "after production.") %
            "\n\n" %
            tr("Check the DRC settings and increase the text height if "
               "needed."),
        "minimum_text_height_violation", locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", device->uuid);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", st.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumWidthViolation
 ******************************************************************************/

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const Data::Segment& segment, const Data::Trace& trace,
    const UnsignedLength& minWidth, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Trace width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(trace.layer->getNameTr(), trace.width->toMmString(),
                 minWidth->toMmString(), "mm"),
        tr("The trace is thinner than the minimum copper width configured in "
           "the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the trace width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", segment.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("trace", trace.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const Data::Plane& plane, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Min. plane width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(plane.layer->getNameTr(), plane.minWidth->toMmString(),
                 minWidth->toMmString(), "mm"),
        tr("The configured minimum width of the plane is smaller than the "
           "minimum copper width configured in the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the minimum plane width in "
               "its properties if needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("plane", plane.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const Data::Polygon& polygon, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Polygon width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(polygon.layer->getNameTr(), polygon.lineWidth->toMmString(),
                 minWidth->toMmString(), "mm"),
        tr("The polygon line width is smaller than the minimum width "
           "configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the polygon line width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const Data::StrokeText& text,
    const BoardDesignRuleCheckData::Device* device,
    const UnsignedLength& minWidth, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Stroke width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(text.layer->getNameTr(), text.strokeWidth->toMmString(),
                 minWidth->toMmString(), "mm"),
        tr("The text stroke width is smaller than the minimum width configured "
           "in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the text stroke width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", device->uuid);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", text.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BoardDesignRuleCheckData::Device& device,
    const Data::Polygon& polygon, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Polygon width of '%1' on '%2': %3 < %4 %5",
           "Placeholders: Device name, layer name, actual width, minimum "
           "width, unit")
            .arg(device.cmpInstanceName,
                 Transform(device.position, device.rotation, device.mirror)
                     .map(*polygon.layer)
                     .getNameTr(),
                 polygon.lineWidth->toMmString(), minWidth->toMmString(), "mm"),
        tr("The polygon line width is smaller than the minimum width "
           "configured "
           "in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the polygon line width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BoardDesignRuleCheckData::Device& device, const Data::Circle& circle,
    const UnsignedLength& minWidth, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Circle width of '%1' on '%2': %3 < %4 %5",
           "Placeholders: Device name, layer name, actual width, minimum "
           "width, unit")
            .arg(device.cmpInstanceName,
                 Transform(device.position, device.rotation, device.mirror)
                     .map(*circle.layer)
                     .getNameTr(),
                 circle.lineWidth->toMmString(), minWidth->toMmString(), "mm"),
        tr("The circle line width is smaller than the minimum width configured "
           "in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the circle line width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgCopperCopperClearanceViolation
 ******************************************************************************/

QString DrcMsgCopperCopperClearanceViolation::Object::getName() const {
  QString name;
  if (!mNetName.isEmpty()) {
    name += "'" % mNetName % "' ";
  }
  if (mPad && mDevice) {
    name = "'" % mDevice->cmpInstanceName;
    if (!mPad->libPkgPadName.isEmpty()) {
      name += ":" % mPad->libPkgPadName;
    }
    name += "'";
  } else if (mTrace) {
    name += tr("trace");
  } else if (mVia) {
    name += tr("via");
  } else if (mPlane) {
    name += tr("plane");
  } else if (mPolygon) {
    name += tr("polygon");
  } else if (mCircle) {
    name += tr("circle");
  } else if (mStrokeText) {
    name += tr("text");
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown copper clearance object.");
  }
  return name;
}

void DrcMsgCopperCopperClearanceViolation::Object::serialize(
    SExpression& node) const {
  if (mPad && mDevice) {
    node.ensureLineBreak();
    node.appendChild("device", mDevice->uuid);
    node.ensureLineBreak();
    node.appendChild("pad", mPad->uuid);
    node.ensureLineBreak();
  } else if (mTrace && mSegment) {
    node.ensureLineBreak();
    node.appendChild("netsegment", mSegment->uuid);
    node.ensureLineBreak();
    node.appendChild("trace", mTrace->uuid);
    node.ensureLineBreak();
  } else if (mVia && mSegment) {
    node.ensureLineBreak();
    node.appendChild("netsegment", mSegment->uuid);
    node.ensureLineBreak();
    node.appendChild("via", mVia->uuid);
    node.ensureLineBreak();
  } else if (mPlane) {
    node.appendChild("plane", mPlane->uuid);
  } else if (mPolygon) {
    if (mDevice) {
      node.ensureLineBreak();
      node.appendChild("device", mDevice->uuid);
      node.ensureLineBreak();
      node.appendChild("polygon", mPolygon->uuid);
      node.ensureLineBreak();
    } else {
      node.appendChild("polygon", mPolygon->uuid);
    }
  } else if (mCircle) {
    if (mDevice) {
      node.ensureLineBreak();
      node.appendChild("device", mDevice->uuid);
      node.ensureLineBreak();
      node.appendChild("circle", mCircle->uuid);
      node.ensureLineBreak();
    } else {
      node.appendChild("circle", mCircle->uuid);
    }
  } else if (mStrokeText) {
    if (mDevice) {
      node.ensureLineBreak();
      node.appendChild("device", mDevice->uuid);
      node.ensureLineBreak();
      node.appendChild("stroke_text", mStrokeText->uuid);
      node.ensureLineBreak();
    } else {
      node.appendChild("stroke_text", mStrokeText->uuid);
    }
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown copper clearance object.");
  }
}

DrcMsgCopperCopperClearanceViolation::DrcMsgCopperCopperClearanceViolation(
    const Object& obj1, const Object& obj2, const QSet<const Layer*>& layers,
    const Length& minClearance, const QVector<Path>& locations)
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance on %1: %2 ↔ %3 < %4 %5",
           "Placeholders: Layer name, object name, object name, "
           "Clearance value, unit")
            .arg(getLayerName(layers), obj1.getName(), obj2.getName(),
                 minClearance.toMmString(), "mm"),
        tr("The clearance between two copper objects of different nets is "
           "smaller than the minimum copper clearance configured in the DRC "
           "settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and move the objects to increase their "
               "clearance if needed."),
        "copper_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  SExpression& node1 = mApproval->appendList("object");
  mApproval->ensureLineBreak();
  SExpression& node2 = mApproval->appendList("object");
  mApproval->ensureLineBreak();

  // Sort nodes to make the approval canonical.
  obj1.serialize(node1);
  obj2.serialize(node2);
  if (node2 < node1) {
    std::swap(node1, node2);
  }
}

QString DrcMsgCopperCopperClearanceViolation::getLayerName(
    const QSet<const Layer*>& layers) {
  if (layers.count() == 1) {
    return "'" % (*layers.begin())->getNameTr() % "'";
  } else {
    return tr("%1 layers", "Placeholder is a number > 1.").arg(layers.count());
  }
}

/*******************************************************************************
 *  DrcMsgCopperBoardClearanceViolation
 ******************************************************************************/

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const Data::Segment& segment, const Data::Via& via,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error,
                     tr("Clearance via ↔ board outline < %1 %2",
                        "Placeholders: Clearance value, unit")
                         .arg(minClearance->toMmString(), "mm"),
                     tr("The clearance between a via and the board outline is "
                        "smaller than the board outline clearance configured "
                        "in the DRC settings.") %
                         " " % seriousTroublesTr() % "\n\n" %
                         tr("Check the DRC settings and move the via away from "
                            "the board outline if needed."),
                     "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", segment.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const Data::Segment& segment, const Data::Trace& trace,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance trace ↔ board outline < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a trace and the board outline is smaller "
           "than the board outline clearance configured in the DRC "
           "settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and move the trace away from the board "
               "outline if needed."),
        "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", segment.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("trace", trace.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BoardDesignRuleCheckData::Device& device,
    const BoardDesignRuleCheckData::Pad& pad,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance pad ↔ board outline < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a footprint pad and the board outline is "
           "smaller than the board outline clearance configured in the DRC "
           "settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and move the device away from the board "
               "outline if needed."),
        "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const Data::Plane& plane, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance plane ↔ board outline < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a plane and the board outline is smaller "
           "than the board outline clearance configured in the DRC "
           "settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the configured plane "
               "clearance if needed."),
        "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("plane", plane.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const Data::Polygon& polygon,
    const BoardDesignRuleCheckData::Device* device,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance copper polygon ↔ board outline < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a polygon and the board outline is smaller "
           "than the board outline clearance configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and move the polygon away from the "
               "board outline if needed."),
        "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", device->uuid);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("polygon", polygon.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BoardDesignRuleCheckData::Device& device, const Data::Circle& circle,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance copper circle ↔ board outline < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a circle and the board outline is smaller "
           "than the board outline clearance configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and move the circle away from the board "
               "outline if needed."),
        "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const Data::StrokeText& strokeText,
    const BoardDesignRuleCheckData::Device* device,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance copper text ↔ board outline < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a stroke text and the board outline is "
           "smaller than the board outline clearance configured in the DRC "
           "settings.") %
            "\n\n" %
            tr("Check the DRC settings and move the stroke text away from the "
               "board outline if needed."),
        "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", device->uuid);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", strokeText.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgCopperHoleClearanceViolation
 ******************************************************************************/

DrcMsgCopperHoleClearanceViolation::DrcMsgCopperHoleClearanceViolation(
    const BoardDesignRuleCheckData::Hole& hole,
    const BoardDesignRuleCheckData::Device* device,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance copper ↔ hole < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a non-plated hole and copper objects is "
           "smaller than the hole clearance configured in the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and move the copper objects away from "
               "the hole if needed."),
        "copper_hole_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", device->uuid);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("hole", hole.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgCopperInKeepoutZone
 ******************************************************************************/

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const BoardDesignRuleCheckData::Device& device,
    const BoardDesignRuleCheckData::Pad& pad,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pad in copper keepout zone: '%1'", "Placeholder is pad name")
            .arg(pad.libPkgPadName),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const Data::Segment& ns, const Data::Via& via,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Via in copper keepout zone: '%1'", "Placeholder is net name")
            .arg(netNameWithFallback(ns.netName)),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const Data::Segment& ns, const Data::Trace& trace,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Trace in copper keepout zone: '%1'", "Placeholder is net name")
            .arg(netNameWithFallback(ns.netName)),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("trace", trace.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const Data::Polygon& polygon, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, tr("Polygon in copper keepout zone"),
                     getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("polygon", polygon.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const BoardDesignRuleCheckData::Device& device,
    const Data::Polygon& polygon, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Polygon in copper keepout zone: '%1'", "Placeholder is device name")
            .arg(device.cmpInstanceName),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const BoardDesignRuleCheckData::Device& device, const Data::Circle& circle,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Circle in copper keepout zone: '%1'", "Placeholder is device name")
            .arg(device.cmpInstanceName),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

void DrcMsgCopperInKeepoutZone::addZoneApprovalNodes(
    const Data::Zone& zone,
    const BoardDesignRuleCheckData::Device* zoneDevice) noexcept {
  mApproval->ensureLineBreak();
  mApproval->appendChild("zone", zone.uuid);
  if (zoneDevice) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("from_device", zoneDevice->uuid);
  }
}

QString DrcMsgCopperInKeepoutZone::getDescription() noexcept {
  return tr("There is a copper object within a copper keepout zone.") % "\n\n" %
      tr("Move the object to outside the keepout zone.");
}

/*******************************************************************************
 *  DrcMsgDrillDrillClearanceViolation
 ******************************************************************************/

DrcMsgDrillDrillClearanceViolation::DrcMsgDrillDrillClearanceViolation(
    const DrcHoleRef& hole1, const DrcHoleRef& hole2,
    const UnsignedLength& minClearance, const QVector<Path>& locations)
  : RuleCheckMessage(Severity::Error,
                     tr("Clearance drill ↔ drill < %1 %2",
                        "Placeholders: Clearance value, unit")
                         .arg(minClearance->toMmString(), "mm"),
                     tr("The clearance between two drills is smaller than the "
                        "drill clearance configured in the DRC settings.") %
                         " " % seriousTroublesTr() % "\n\n" %
                         tr("Check the DRC settings and move the drills to "
                            "increase their distance if needed."),
                     "drill_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  SExpression& node1 = mApproval->appendList("drill");
  mApproval->ensureLineBreak();
  SExpression& node2 = mApproval->appendList("drill");
  mApproval->ensureLineBreak();

  // Sort nodes to make the approval canonical.
  hole1.serialize(node1);
  hole2.serialize(node2);
  if (node2 < node1) {
    std::swap(node1, node2);
  }
}

/*******************************************************************************
 *  DrcMsgDrillBoardClearanceViolation
 ******************************************************************************/

DrcMsgDrillBoardClearanceViolation::DrcMsgDrillBoardClearanceViolation(
    const DrcHoleRef& hole, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance drill ↔ board outline < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a drill and the board outline is smaller "
           "than the drill clearance configured in the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and move the drill away from the board "
               "outline if needed."),
        "drill_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  hole.serialize(*mApproval);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgDeviceInCourtyard
 ******************************************************************************/

DrcMsgDeviceInCourtyard::DrcMsgDeviceInCourtyard(
    const BoardDesignRuleCheckData::Device& device1,
    const BoardDesignRuleCheckData::Device& device2,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Device in courtyard: '%1' ↔ '%2'",
           "Placeholders: Device 1 name, device 2 name")
            .arg(std::min(device1.cmpInstanceName, device2.cmpInstanceName),
                 std::max(device1.cmpInstanceName, device2.cmpInstanceName)),
        tr("A device is placed within the courtyard of another device, which "
           "might cause troubles during assembly of these parts.") %
            "\n\n" %
            tr("Either move the devices to increase their clearance or approve "
               "this message if you're sure they can be assembled without "
               "problems."),
        "device_in_courtyard", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", std::min(device1.uuid, device2.uuid));
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", std::max(device1.uuid, device2.uuid));
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgOverlappingDevices
 ******************************************************************************/

DrcMsgOverlappingDevices::DrcMsgOverlappingDevices(
    const BoardDesignRuleCheckData::Device& device1,
    const BoardDesignRuleCheckData::Device& device2,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Device overlap: '%1' ↔ '%2'",
           "Placeholders: Device 1 name, device 2 name")
            .arg(std::min(device1.cmpInstanceName, device2.cmpInstanceName),
                 std::max(device1.cmpInstanceName, device2.cmpInstanceName)),
        tr("Two devices are overlapping and thus probably cannot be assembled "
           "both at the same time.") %
            "\n\n" %
            tr("Either move the devices to increase their clearance or approve "
               "this message if you're sure they can be assembled without "
               "problems (or only one of them gets assembled)."),
        "overlapping_devices", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", std::min(device1.uuid, device2.uuid));
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", std::max(device1.uuid, device2.uuid));
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgExposureInKeepoutZone
 ******************************************************************************/

DrcMsgDeviceInKeepoutZone::DrcMsgDeviceInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const BoardDesignRuleCheckData::Device& device,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Device in keepout zone: '%1'", "Placeholder is device name")
            .arg(device.cmpInstanceName),
        getDescription(), "device_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

void DrcMsgDeviceInKeepoutZone::addZoneApprovalNodes(
    const Data::Zone& zone,
    const BoardDesignRuleCheckData::Device* zoneDevice) noexcept {
  mApproval->ensureLineBreak();
  mApproval->appendChild("zone", zone.uuid);
  if (zoneDevice) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("from_device", zoneDevice->uuid);
  }
}

QString DrcMsgDeviceInKeepoutZone::getDescription() noexcept {
  return tr("There is a device within a keepout zone.") % "\n\n" %
      tr("Move the device to outside the keepout zone.");
}

/*******************************************************************************
 *  DrcMsgExposureInKeepoutZone
 ******************************************************************************/

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const BoardDesignRuleCheckData::Device& device,
    const BoardDesignRuleCheckData::Pad& pad,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pad in exposure keepout zone: '%1'", "Placeholder is pad name")
            .arg(pad.libPkgPadName),
        getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const Data::Segment& ns, const Data::Via& via,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Via in exposure keepout zone: '%1'", "Placeholder is net name")
            .arg(netNameWithFallback(ns.netName)),
        getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const Data::Polygon& polygon, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, tr("Polygon in exposure keepout zone"),
                     getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("polygon", polygon.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const BoardDesignRuleCheckData::Device& device,
    const Data::Polygon& polygon, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error,
                     tr("Polygon in exposure keepout zone: '%1'",
                        "Placeholder is device name")
                         .arg(device.cmpInstanceName),
                     getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const Data::Zone& zone, const BoardDesignRuleCheckData::Device* zoneDevice,
    const BoardDesignRuleCheckData::Device& device, const Data::Circle& circle,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error,
                     tr("Circle in exposure keepout zone: '%1'",
                        "Placeholder is device name")
                         .arg(device.cmpInstanceName),
                     getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.uuid);
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(zone, zoneDevice);
  mApproval->ensureLineBreak();
}

void DrcMsgExposureInKeepoutZone::addZoneApprovalNodes(
    const Data::Zone& zone,
    const BoardDesignRuleCheckData::Device* zoneDevice) noexcept {
  mApproval->ensureLineBreak();
  mApproval->appendChild("zone", zone.uuid);
  if (zoneDevice) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("from_device", zoneDevice->uuid);
  }
}

QString DrcMsgExposureInKeepoutZone::getDescription() noexcept {
  return tr("There is a solder resist opening within an exposure keepout "
            "zone.") %
      "\n\n" % tr("Move the object to outside the keepout zone.");
}

/*******************************************************************************
 *  DrcMsgMinimumAnnularRingViolation
 ******************************************************************************/

DrcMsgMinimumAnnularRingViolation::DrcMsgMinimumAnnularRingViolation(
    const Data::Segment& ns, const Data::Via& via,
    const UnsignedLength& minAnnularWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Via annular ring of '%1' < %2 %3",
           "Placeholders: Net name, minimum annular width, unit")
            .arg(netNameWithFallback(ns.netName), minAnnularWidth->toMmString(),
                 "mm"),
        tr("The via annular ring width (i.e. the copper around the hole) is "
           "smaller than the minimum annular width configured in the DRC "
           "settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the via size if needed."),
        "minimum_annular_ring_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.uuid);
  mApproval->ensureLineBreak();
}

DrcMsgMinimumAnnularRingViolation::DrcMsgMinimumAnnularRingViolation(
    const BoardDesignRuleCheckData::Device& device,
    const BoardDesignRuleCheckData::Pad& pad,
    const UnsignedLength& minAnnularWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pad annular ring of '%1' < %2 %3",
           "Placeholders: Net name, minimum annular width, unit")
            .arg(device.cmpInstanceName + "::" + pad.libPkgPadName,
                 minAnnularWidth->toMmString(), "mm"),
        tr("The through-hole pad annular ring width (i.e. the copper around "
           "the hole) is smaller than the minimum annular width configured in "
           "the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the pad size if needed."),
        "minimum_annular_ring_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumDrillDiameterViolation
 ******************************************************************************/

DrcMsgMinimumDrillDiameterViolation::DrcMsgMinimumDrillDiameterViolation(
    const DrcHoleRef& hole, const UnsignedLength& minDiameter,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, determineMessage(hole, minDiameter),
                     determineDescription(hole),
                     "minimum_drill_diameter_violation", locations) {
  mApproval->ensureLineBreak();
  hole.serialize(*mApproval);
  mApproval->ensureLineBreak();
}

QString DrcMsgMinimumDrillDiameterViolation::determineMessage(
    const DrcHoleRef& hole, const UnsignedLength& minDiameter) noexcept {
  if (hole.isViaHole()) {
    return tr("Via drill diameter of '%1': %2 < %3 %4",
              "Placeholders: Net name, actual diameter, minimum diameter")
        .arg(netNameWithFallback(hole.getNetName()),
             hole.getDiameter()->toMmString(), minDiameter->toMmString(), "mm");
  } else if (const BoardDesignRuleCheckData::Pad* pad = hole.getPad()) {
    return tr("Pad drill diameter of '%1': %2 < %3 %4",
              "Placeholders: Net name, actual diameter, minimum diameter")
        .arg(pad->libPkgPadName, hole.getDiameter()->toMmString(),
             minDiameter->toMmString(), "mm");
  } else {
    return tr("NPTH drill diameter: %1 < %2 %3",
              "Placeholders: Actual diameter, minimum diameter, unit")
        .arg(hole.getDiameter()->toMmString(), minDiameter->toMmString(), "mm");
  }
}

QString DrcMsgMinimumDrillDiameterViolation::determineDescription(
    const DrcHoleRef& hole) noexcept {
  QString s;
  if (hole.isViaHole()) {
    s +=
        tr("The drill diameter of the via is smaller than the minimum plated "
           "drill diameter configured in the DRC settings.");
  } else if (hole.isPadHole()) {
    s +=
        tr("The drill diameter of the through-hole pad is smaller than the "
           "minimum plated drill diameter configured in the DRC settings.");
  } else {
    s +=
        tr("The drill diameter of the non-plated hole is smaller than the "
           "minimum non-plated drill diameter configured in the DRC settings.");
  }
  s += "\n\n";
  s += tr("Check the DRC settings and increase the drill diameter if needed.");
  return s;
}

/*******************************************************************************
 *  DrcMsgMinimumSlotWidthViolation
 ******************************************************************************/

DrcMsgMinimumSlotWidthViolation::DrcMsgMinimumSlotWidthViolation(
    const DrcHoleRef& hole, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        determineMessage(hole.isPlated(), hole.getDiameter(), minWidth),
        determineDescription(hole.isPlated()), "minimum_slot_width_violation",
        locations) {
  mApproval->ensureLineBreak();
  hole.serialize(*mApproval);
  mApproval->ensureLineBreak();
}

QString DrcMsgMinimumSlotWidthViolation::determineMessage(
    bool plated, const PositiveLength& actualWidth,
    const UnsignedLength& minWidth) noexcept {
  if (plated) {
    return tr("Plated slot width: %1 < %2 %3",
              "Placeholders: Actual width, minimum width, unit")
        .arg(actualWidth->toMmString(), minWidth->toMmString(), "mm");
  } else {
    return tr("NPTH slot width: %1 < %2 %3",
              "Placeholders: Actual width, minimum width, unit")
        .arg(actualWidth->toMmString(), minWidth->toMmString(), "mm");
  }
}

QString DrcMsgMinimumSlotWidthViolation::determineDescription(
    bool plated) noexcept {
  QString s;
  if (plated) {
    s +=
        tr("The width of the plated slot is smaller than the minimum plated "
           "slot width configured in the DRC settings.");
  } else {
    s +=
        tr("The width of the non-plated slot is smaller than the "
           "minimum non-plated slot width configured in the DRC settings.");
  }
  s += "\n\n";
  s += tr("Check the DRC settings and increase the slot width if needed.");
  return s;
}

/*******************************************************************************
 *  DrcMsgInvalidPadConnection
 ******************************************************************************/

DrcMsgInvalidPadConnection::DrcMsgInvalidPadConnection(
    const BoardDesignRuleCheckData::Device& device,
    const BoardDesignRuleCheckData::Pad& pad, const Layer& layer,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Invalid connection of pad '%1' on '%2'",
           "Placeholders: Pad name, layer name")
            .arg(pad.libPkgPadName),
        tr("The pad origin must be located within the pads copper area, "
           "or for THT pads within a hole. Otherwise traces might not be"
           "connected fully. This issue needs to be fixed in the library."),
        "invalid_pad_connection", locations) {
  mApproval->appendChild("layer", layer);
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgForbiddenSlotUsed
 ******************************************************************************/

DrcMsgForbiddenSlot::DrcMsgForbiddenSlot(
    const BoardDesignRuleCheckData::Hole& hole,
    const BoardDesignRuleCheckData::Device* device,
    const BoardDesignRuleCheckData::Pad* pad,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, determineMessage(hole.path),
                     determineDescription(hole.path), "forbidden_slot",
                     locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", device->uuid);
    mApproval->ensureLineBreak();
  }
  if (pad) {
    mApproval->appendChild("pad", pad->uuid);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("hole", hole.uuid);
  mApproval->ensureLineBreak();
}

QString DrcMsgForbiddenSlot::determineMessage(
    const NonEmptyPath& path) noexcept {
  if (path->isCurved()) {
    return tr("Hole is a slot with curves");
  } else if (path->getVertices().count() > 2) {
    return tr("Hole is a multi-segment slot");
  } else {
    return tr("Hole is a slot");
  }
}

QString DrcMsgForbiddenSlot::determineDescription(
    const NonEmptyPath& path) noexcept {
  const QString suggestion = "\n" %
      tr("Either avoid them or check if your PCB manufacturer supports them.");
  const QString checkSlotMode = "\n" %
      tr("Choose the desired Excellon slot mode when generating the production "
         "data (G85 vs. G00..G03).");
  const QString g85NotAvailable = "\n" %
      tr("The drilled slot mode (G85) will not be available when generating "
         "production data.");

  if (path->isCurved()) {
    return tr("Curved slots are a very unusual thing and may cause troubles "
              "with many PCB manufacturers.") %
        suggestion % g85NotAvailable;
  } else if (path->getVertices().count() > 2) {
    return tr("Multi-segment slots are a rather unusual thing and may cause "
              "troubles with some PCB manufacturers.") %
        suggestion % checkSlotMode;
  } else {
    return tr("Slots may cause troubles with some PCB manufacturers.") %
        suggestion % checkSlotMode;
  }
}

/*******************************************************************************
 *  DrcMsgForbiddenVia
 ******************************************************************************/

DrcMsgForbiddenVia::DrcMsgForbiddenVia(
    const BoardDesignRuleCheckData::Segment& ns, const Data::Via& via,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, determineMessage(ns, via),
                     determineDescription(via), "forbidden_via", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.uuid);
  mApproval->ensureLineBreak();
}

QString DrcMsgForbiddenVia::determineMessage(
    const BoardDesignRuleCheckData::Segment& ns,
    const Data::Via& via) noexcept {
  if (via.isBlind) {
    return tr("Blind via in net '%1'").arg(netNameWithFallback(ns.netName));
  } else {
    return tr("Buried via in net '%1'").arg(netNameWithFallback(ns.netName));
  }
}

QString DrcMsgForbiddenVia::determineDescription(
    const Data::Via& via) noexcept {
  const QString suggestion = "\n" %
      tr("Either avoid them or check if your PCB manufacturer supports them "
         "and adjust the DRC settings accordingly.");
  if (via.isBlind) {
    return tr("Blind vias are expensive to manufacture and not every PCB "
              "manufacturer is able to create them.") %
        suggestion;
  } else {
    return tr("Buried vias are expensive to manufacture and not every PCB "
              "manufacturer is able to create them.") %
        suggestion;
  }
}

/*******************************************************************************
 *  Class DrcMsgInvalidVia
 ******************************************************************************/

DrcMsgInvalidVia::DrcMsgInvalidVia(const BoardDesignRuleCheckData::Segment& ns,
                                   const Data::Via& via,
                                   const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     tr("Invalid via in net '%1'", "Placeholders: Net name")
                         .arg(netNameWithFallback(ns.netName)),
                     tr("The via is only drilled between one layer and is "
                        "therefore invalid."),
                     "invalid_via", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  Class DrcMsgSilkscreenClearanceViolation
 ******************************************************************************/

DrcMsgSilkscreenClearanceViolation::DrcMsgSilkscreenClearanceViolation(
    const Data::StrokeText& st, const BoardDesignRuleCheckData::Device* device,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance silkscreen text ↔ stop mask < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a silkscreen text and a solder resist "
           "opening is smaller than the minimum clearance configured in the "
           "DRC settings. This could lead to clipped silkscreen during "
           "production.") %
            "\n\n" %
            tr("Check the DRC settings and move the text away from the "
               "solder resist opening if needed."),
        "silkscreen_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  if (device) {
    mApproval->appendChild("device", device->uuid);
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", st.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgUselessZone
 ******************************************************************************/

DrcMsgUselessZone::DrcMsgUselessZone(const Data::Zone& zone,
                                     const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Useless zone"),
        tr("The zone has no layer or rule enabled so it is useless."),
        "useless_zone", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("zone", zone.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgUselessVia
 ******************************************************************************/

DrcMsgUselessVia::DrcMsgUselessVia(const BoardDesignRuleCheckData::Segment& ns,
                                   const Data::Via& via,
                                   const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     tr("Useless via in net '%1'", "Placeholders: Net name")
                         .arg(netNameWithFallback(ns.netName)),
                     tr("The via is connected on less than two layers, thus it "
                        "seems to be useless."),
                     "useless_via", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", ns.uuid);
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.uuid);
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgDisabledLayer
 ******************************************************************************/

DrcMsgDisabledLayer::DrcMsgDisabledLayer(const Layer& layer) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Objects on disabled layer: '%1'").arg(layer.getNameTr()),
        tr("The layer contains copper objects, but it is disabled in the board "
           "setup dialog and thus will be ignored in any production data "
           "exports. Either increase the layer count to get this layer "
           "exported, or remove all objects on this layer (by temporarily "
           "enabling this layer to see them)."),
        "disabled_layer") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("layer", layer.getId());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgUnusedLayer
 ******************************************************************************/

DrcMsgUnusedLayer::DrcMsgUnusedLayer(const Layer& layer) noexcept
  : RuleCheckMessage(
        Severity::Hint, tr("Unused layer: '%1'").arg(layer.getNameTr()),
        tr("The layer contains no copper objects (except the automatically "
           "generated through-hole annular rings, if any) so it is useless. "
           "This is not critical, but if your intention is to flood it with "
           "copper, you need to add a plane manually. Or if you don't need "
           "this layer, you might want to reduce the layer count in the board "
           "setup dialog to avoid unnecessary production costs. Also some PCB "
           "manufacturers might be confused by empty layers."),
        "unused_layer") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("layer", layer.getId());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
