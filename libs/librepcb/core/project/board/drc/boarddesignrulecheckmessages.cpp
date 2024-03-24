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

/*******************************************************************************
 *  DrcMsgMissingDevice
 ******************************************************************************/

DrcMsgMissingDevice::DrcMsgMissingDevice(
    const ComponentInstance& component) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Missing device: '%1'", "Placeholders: Device name")
            .arg(*component.getName()),
        tr("There's a component in the schematics without a corresponding "
           "device in the board, so the circuit of the PCB is not "
           "complete.\n\nUse the \"Place Devices\" dock to add the device."),
        "missing_device", {}) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", component.getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMissingConnection
 ******************************************************************************/

DrcMsgMissingConnection::DrcMsgMissingConnection(const BI_NetLineAnchor& p1,
                                                 const BI_NetLineAnchor& p2,
                                                 const NetSignal& netSignal,
                                                 const QVector<Path>& locations)
  : RuleCheckMessage(
        Severity::Error,
        tr("Missing connection in '%1': %2 ↔ %3",
           "Placeholders: Net name, connection count")
            .arg(*netSignal.getName(), getAnchorName(p1), getAnchorName(p2)),
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
  serializeAnchor(fromNode, p1);
  serializeAnchor(toNode, p2);
  if (toNode < fromNode) {
    std::swap(fromNode, toNode);
  }
  fromNode.setName("from");
  toNode.setName("to");
}

QString DrcMsgMissingConnection::getAnchorName(const BI_NetLineAnchor& anchor) {
  QString name;
  if (dynamic_cast<const BI_Via*>(&anchor)) {
    name = tr("Via");
  } else if (const BI_FootprintPad* pad =
                 dynamic_cast<const BI_FootprintPad*>(&anchor)) {
    name = "'" % (*pad->getDevice().getComponentInstance().getName());
    if (const PackagePad* pkgPad = pad->getLibPackagePad()) {
      name += ":" % *pkgPad->getName();
    }
    name += "'";
  } else if (dynamic_cast<const BI_NetPoint*>(&anchor)) {
    name = tr("Trace");
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown airwire anchor type.");
  }
  return name;
}

void DrcMsgMissingConnection::serializeAnchor(SExpression& node,
                                              const BI_NetLineAnchor& anchor) {
  if (const BI_Via* via = dynamic_cast<const BI_Via*>(&anchor)) {
    node.appendChild("netsegment", via->getNetSegment().getUuid());
  } else if (const BI_FootprintPad* pad =
                 dynamic_cast<const BI_FootprintPad*>(&anchor)) {
    node.ensureLineBreak();
    node.appendChild("device", pad->getDevice().getComponentInstanceUuid());
    node.ensureLineBreak();
    node.appendChild("pad", pad->getLibPadUuid());
    node.ensureLineBreak();
  } else if (const BI_NetPoint* netPoint =
                 dynamic_cast<const BI_NetPoint*>(&anchor)) {
    node.appendChild("netsegment", netPoint->getNetSegment().getUuid());
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown airwire anchor type.");
  }
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
    const BI_Device* device, const Uuid& polygon,
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
    mApproval->appendChild("device", device->getComponentInstanceUuid());
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

DrcMsgEmptyNetSegment::DrcMsgEmptyNetSegment(
    const BI_NetSegment& netSegment) noexcept
  : RuleCheckMessage(Severity::Hint,
                     tr("Empty segment of net '%1': '%2'",
                        "Placeholders: Net name, segment UUID")
                         .arg(netSegment.getNetNameToDisplay(true),
                              netSegment.getUuid().toStr()),
                     "There's a net segment in the board without any via or "
                     "trace. This should not happen, please report it as a "
                     "bug. But no worries, this issue is not harmful at all "
                     "so you can safely ignore this message.",
                     "empty_netsegment", {}) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", netSegment.getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgUnconnectedJunction
 ******************************************************************************/

DrcMsgUnconnectedJunction::DrcMsgUnconnectedJunction(
    const BI_NetPoint& netPoint, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Hint,
        tr("Unconnected junction in net: '%1'")
            .arg(netPoint.getNetSegment().getNetNameToDisplay(true)),
        "There's an invisible junction in the board without any trace "
        "attached. This should not happen, please report it as a bug. But "
        "no worries, this issue is not harmful at all so you can safely "
        "ignore this message.",
        "unconnected_junction", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("board", netPoint.getBoard().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", netPoint.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("junction", netPoint.getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumTextHeightViolation
 ******************************************************************************/

DrcMsgMinimumTextHeightViolation::DrcMsgMinimumTextHeightViolation(
    const BI_StrokeText& text, const UnsignedLength& minHeight,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Text height on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual height, minimum height, unit")
            .arg(text.getData().getLayer().getNameTr(),
                 text.getData().getHeight()->toMmString(),
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
  if (const BI_Device* device = text.getDevice()) {
    mApproval->appendChild("device", device->getComponentInstanceUuid());
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", text.getData().getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumWidthViolation
 ******************************************************************************/

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_NetLine& netLine, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Trace width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(netLine.getLayer().getNameTr(),
                 netLine.getWidth()->toMmString(), minWidth->toMmString(),
                 "mm"),
        tr("The trace is thinner than the minimum copper width configured in "
           "the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the trace width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", netLine.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("trace", netLine.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_Plane& plane, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Min. plane width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(plane.getLayer().getNameTr(),
                 plane.getMinWidth()->toMmString(), minWidth->toMmString(),
                 "mm"),
        tr("The configured minimum width of the plane is smaller than the "
           "minimum copper width configured in the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the minimum plane width in "
               "its properties if needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("plane", plane.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_Polygon& polygon, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Polygon width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(polygon.getData().getLayer().getNameTr(),
                 polygon.getData().getLineWidth()->toMmString(),
                 minWidth->toMmString(), "mm"),
        tr("The polygon line width is smaller than the minimum width "
           "configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the polygon line width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_StrokeText& text, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Stroke width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(text.getData().getLayer().getNameTr(),
                 text.getData().getStrokeWidth()->toMmString(),
                 minWidth->toMmString(), "mm"),
        tr("The text stroke width is smaller than the minimum width configured "
           "in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the text stroke width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  if (const BI_Device* device = text.getDevice()) {
    mApproval->appendChild("device", device->getComponentInstanceUuid());
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", text.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_Device& device, const Polygon& polygon,
    const UnsignedLength& minWidth, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Polygon width of '%1' on '%2': %3 < %4 %5",
           "Placeholders: Device name, layer name, actual width, minimum "
           "width, unit")
            .arg(*device.getComponentInstance().getName(),
                 Transform(device).map(polygon.getLayer()).getNameTr(),
                 polygon.getLineWidth()->toMmString(), minWidth->toMmString(),
                 "mm"),
        tr("The polygon line width is smaller than the minimum width "
           "configured "
           "in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the polygon line width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_Device& device, const Circle& circle,
    const UnsignedLength& minWidth, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Circle width of '%1' on '%2': %3 < %4 %5",
           "Placeholders: Device name, layer name, actual width, minimum "
           "width, unit")
            .arg(*device.getComponentInstance().getName(),
                 Transform(device).map(circle.getLayer()).getNameTr(),
                 circle.getLineWidth()->toMmString(), minWidth->toMmString(),
                 "mm"),
        tr("The circle line width is smaller than the minimum width configured "
           "in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the circle line width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgCopperCopperClearanceViolation
 ******************************************************************************/

DrcMsgCopperCopperClearanceViolation::DrcMsgCopperCopperClearanceViolation(
    const NetSignal* net1, const BI_Base& item1, const Polygon* polygon1,
    const Circle* circle1, const NetSignal* net2, const BI_Base& item2,
    const Polygon* polygon2, const Circle* circle2,
    const QVector<const Layer*>& layers, const Length& minClearance,
    const QVector<Path>& locations)
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance on %1: %2 ↔ %3 < %4 %5",
           "Placeholders: Layer name, object name, object name, "
           "Clearance value, unit")
            .arg(getLayerName(layers),
                 getObjectName(net1, item1, polygon1, circle1),
                 getObjectName(net2, item2, polygon2, circle2),
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
  serializeObject(node1, item1, polygon1, circle1);
  serializeObject(node2, item2, polygon2, circle2);
  if (node2 < node1) {
    std::swap(node1, node2);
  }
}

QString DrcMsgCopperCopperClearanceViolation::getLayerName(
    const QVector<const Layer*>& layers) {
  if (layers.count() == 1) {
    return "'" % layers.first()->getNameTr() % "'";
  } else {
    return tr("%1 layers", "Placeholder is a number > 1.").arg(layers.count());
  }
}

QString DrcMsgCopperCopperClearanceViolation::getObjectName(
    const NetSignal* net, const BI_Base& item, const Polygon* polygon,
    const Circle* circle) {
  QString name;
  if (net) {
    name += "'" % *net->getName() % "' ";
  }
  if (const BI_FootprintPad* pad =
          dynamic_cast<const BI_FootprintPad*>(&item)) {
    name = "'" % (*pad->getDevice().getComponentInstance().getName());
    if (const PackagePad* pkgPad = pad->getLibPackagePad()) {
      name += ":" % *pkgPad->getName();
    }
    name += "'";
  } else if (dynamic_cast<const BI_Via*>(&item)) {
    name += tr("via");
  } else if (dynamic_cast<const BI_NetLine*>(&item)) {
    name += tr("trace");
  } else if (dynamic_cast<const BI_Plane*>(&item)) {
    name += tr("plane");
  } else if (polygon || dynamic_cast<const BI_Polygon*>(&item)) {
    name += tr("polygon");
  } else if (circle) {
    name += tr("circle");
  } else if (dynamic_cast<const BI_StrokeText*>(&item)) {
    name += tr("text");
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown copper clearance object.");
  }
  return name;
}

void DrcMsgCopperCopperClearanceViolation::serializeObject(
    SExpression& node, const BI_Base& item, const Polygon* polygon,
    const Circle* circle) {
  if (const BI_Device* device = dynamic_cast<const BI_Device*>(&item)) {
    node.ensureLineBreak();
    node.appendChild("device", device->getComponentInstanceUuid());
    if (polygon) {
      node.ensureLineBreak();
      node.appendChild("polygon", polygon->getUuid());
    }
    if (circle) {
      node.ensureLineBreak();
      node.appendChild("circle", circle->getUuid());
    }
    node.ensureLineBreak();
  } else if (const BI_FootprintPad* pad =
                 dynamic_cast<const BI_FootprintPad*>(&item)) {
    node.ensureLineBreak();
    node.appendChild("device", pad->getDevice().getComponentInstanceUuid());
    node.ensureLineBreak();
    node.appendChild("pad", pad->getLibPadUuid());
    node.ensureLineBreak();
  } else if (const BI_Via* via = dynamic_cast<const BI_Via*>(&item)) {
    node.ensureLineBreak();
    node.appendChild("netsegment", via->getNetSegment().getUuid());
    node.ensureLineBreak();
    node.appendChild("via", via->getUuid());
    node.ensureLineBreak();
  } else if (const BI_NetLine* netLine =
                 dynamic_cast<const BI_NetLine*>(&item)) {
    node.ensureLineBreak();
    node.appendChild("netsegment", netLine->getNetSegment().getUuid());
    node.ensureLineBreak();
    node.appendChild("trace", netLine->getUuid());
    node.ensureLineBreak();
  } else if (const BI_Plane* plane = dynamic_cast<const BI_Plane*>(&item)) {
    node.appendChild("plane", plane->getUuid());
  } else if (const BI_Polygon* polygon =
                 dynamic_cast<const BI_Polygon*>(&item)) {
    node.appendChild("polygon", polygon->getData().getUuid());
  } else if (const BI_StrokeText* strokeText =
                 dynamic_cast<const BI_StrokeText*>(&item)) {
    if (const BI_Device* device = strokeText->getDevice()) {
      node.ensureLineBreak();
      node.appendChild("device", device->getComponentInstanceUuid());
      node.ensureLineBreak();
      node.appendChild("stroke_text", strokeText->getData().getUuid());
      node.ensureLineBreak();
    } else {
      node.appendChild("stroke_text", strokeText->getData().getUuid());
    }
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown copper clearance object.");
  }
}

/*******************************************************************************
 *  DrcMsgCopperBoardClearanceViolation
 ******************************************************************************/

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Via& via, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error,
                     tr("Clearance board outline ↔ via < %1 %2",
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
  mApproval->appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_NetLine& netLine, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
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
  mApproval->appendChild("netsegment", netLine.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("trace", netLine.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_FootprintPad& pad, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
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
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Plane& plane, const UnsignedLength& minClearance,
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
  mApproval->appendChild("plane", plane.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Polygon& polygon, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, getPolygonMessage(minClearance),
                     getPolygonDescription(),
                     "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Device& device, const Polygon& polygon,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, getPolygonMessage(minClearance),
                     getPolygonDescription(),
                     "copper_board_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Device& device, const Circle& circle,
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
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_StrokeText& strokeText, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
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
  if (const BI_Device* device = strokeText.getDevice()) {
    mApproval->appendChild("device", device->getComponentInstanceUuid());
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", strokeText.getData().getUuid());
  mApproval->ensureLineBreak();
}

QString DrcMsgCopperBoardClearanceViolation::getPolygonMessage(
    const UnsignedLength& minClearance) noexcept {
  return tr("Clearance copper polygon ↔ board outline < %1 %2",
            "Placeholders: Clearance value, unit")
      .arg(minClearance->toMmString(), "mm");
}

QString DrcMsgCopperBoardClearanceViolation::getPolygonDescription() noexcept {
  return tr("The clearance between a polygon and the board outline is smaller "
            "than the board outline clearance configured in the DRC "
            "settings.") %
      "\n\n" %
      tr("Check the DRC settings and move the polygon away from the "
         "board outline if needed.");
}

/*******************************************************************************
 *  DrcMsgCopperHoleClearanceViolation
 ******************************************************************************/

DrcMsgCopperHoleClearanceViolation::DrcMsgCopperHoleClearanceViolation(
    const BI_Hole& hole, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, getMessage(minClearance),
                     getDescription(), "copper_hole_clearance_violation",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgCopperHoleClearanceViolation::DrcMsgCopperHoleClearanceViolation(
    const BI_Device& device, const Hole& hole,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, getMessage(minClearance),
                     getDescription(), "copper_hole_clearance_violation",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getUuid());
  mApproval->ensureLineBreak();
}

QString DrcMsgCopperHoleClearanceViolation::getMessage(
    const UnsignedLength& minClearance) noexcept {
  return tr("Clearance copper ↔ hole < %1 %2",
            "Placeholders: Clearance value, unit")
      .arg(minClearance->toMmString(), "mm");
}

QString DrcMsgCopperHoleClearanceViolation::getDescription() noexcept {
  return tr("The clearance between a non-plated hole and copper objects is "
            "smaller than the hole clearance configured in the DRC settings.") %
      " " % seriousTroublesTr() % "\n\n" %
      tr("Check the DRC settings and move the copper objects away from "
         "the hole if needed.");
}

/*******************************************************************************
 *  DrcMsgCopperInKeepoutZone
 ******************************************************************************/

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_FootprintPad& pad,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pad in copper keepout zone: '%1'", "Placeholder is pad name")
            .arg(pad.getText()),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Via& via,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Via in copper keepout zone: '%1'", "Placeholder is net name")
            .arg(via.getNetSegment().getNetNameToDisplay(true)),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_NetLine& netLine,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Trace in copper keepout zone: '%1'", "Placeholder is net name")
            .arg(netLine.getNetSegment().getNetNameToDisplay(true)),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("netsegment", netLine.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("trace", netLine.getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Polygon& polygon,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, tr("Polygon in copper keepout zone"),
                     getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("polygon", polygon.getData().getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Device& device, const Polygon& polygon,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Polygon in copper keepout zone: '%1'", "Placeholder is device name")
            .arg(*device.getComponentInstance().getName()),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgCopperInKeepoutZone::DrcMsgCopperInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Device& device, const Circle& circle,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Circle in copper keepout zone: '%1'", "Placeholder is device name")
            .arg(*device.getComponentInstance().getName()),
        getDescription(), "copper_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

void DrcMsgCopperInKeepoutZone::addZoneApprovalNodes(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone) noexcept {
  if (boardZone) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("zone", boardZone->getData().getUuid());
  }
  if (deviceZone) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("zone", deviceZone->getUuid());
  }
  if (zoneDevice) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("from_device",
                           zoneDevice->getComponentInstanceUuid());
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
    const BI_Base& item1, const Uuid& hole1, const BI_Base& item2,
    const Uuid& hole2, const UnsignedLength& minClearance,
    const QVector<Path>& locations)
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
  serializeObject(node1, item1, hole1);
  serializeObject(node2, item2, hole2);
  if (node2 < node1) {
    std::swap(node1, node2);
  }
}

void DrcMsgDrillDrillClearanceViolation::serializeObject(SExpression& node,
                                                         const BI_Base& item,
                                                         const Uuid& hole) {
  if (const BI_Via* via = dynamic_cast<const BI_Via*>(&item)) {
    node.ensureLineBreak();
    node.appendChild("netsegment", via->getNetSegment().getUuid());
    node.ensureLineBreak();
    node.appendChild("via", via->getUuid());
    node.ensureLineBreak();
  } else if (const BI_Hole* boardHole = dynamic_cast<const BI_Hole*>(&item)) {
    node.appendChild("hole", boardHole->getData().getUuid());
  } else if (const BI_FootprintPad* pad =
                 dynamic_cast<const BI_FootprintPad*>(&item)) {
    node.ensureLineBreak();
    node.appendChild("device", pad->getDevice().getComponentInstanceUuid());
    node.ensureLineBreak();
    node.appendChild("pad", pad->getLibPadUuid());
    node.ensureLineBreak();
    node.appendChild("hole", hole);
    node.ensureLineBreak();
  } else if (const BI_Device* device = dynamic_cast<const BI_Device*>(&item)) {
    node.ensureLineBreak();
    node.appendChild("device", device->getComponentInstanceUuid());
    node.ensureLineBreak();
    node.appendChild("hole", hole);
    node.ensureLineBreak();
  } else {
    throw LogicError(__FILE__, __LINE__, "Unknown drill clearance object.");
  }
}

/*******************************************************************************
 *  DrcMsgDrillBoardClearanceViolation
 ******************************************************************************/

DrcMsgDrillBoardClearanceViolation::DrcMsgDrillBoardClearanceViolation(
    const BI_Via& via, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, getMessage(minClearance),
                     getDescription(), "drill_board_clearance_violation",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgDrillBoardClearanceViolation::DrcMsgDrillBoardClearanceViolation(
    const BI_FootprintPad& pad, const PadHole& hole,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, getMessage(minClearance),
                     getDescription(), "drill_board_clearance_violation",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgDrillBoardClearanceViolation::DrcMsgDrillBoardClearanceViolation(
    const BI_Hole& hole, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, getMessage(minClearance),
                     getDescription(), "drill_board_clearance_violation",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgDrillBoardClearanceViolation::DrcMsgDrillBoardClearanceViolation(
    const BI_Device& device, const Hole& hole,
    const UnsignedLength& minClearance, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, getMessage(minClearance),
                     getDescription(), "drill_board_clearance_violation",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getUuid());
  mApproval->ensureLineBreak();
}

QString DrcMsgDrillBoardClearanceViolation::getMessage(
    const UnsignedLength& minClearance) noexcept {
  return tr("Clearance drill ↔ board outline < %1 %2",
            "Placeholders: Clearance value, unit")
      .arg(minClearance->toMmString(), "mm");
}

QString DrcMsgDrillBoardClearanceViolation::getDescription() noexcept {
  return tr("The clearance between a drill and the board outline is smaller "
            "than the drill clearance configured in the DRC settings.") %
      " " % seriousTroublesTr() % "\n\n" %
      tr("Check the DRC settings and move the drill away from the board "
         "outline if needed.");
}

/*******************************************************************************
 *  DrcMsgDeviceInCourtyard
 ******************************************************************************/

DrcMsgDeviceInCourtyard::DrcMsgDeviceInCourtyard(
    const BI_Device& device1, const BI_Device& device2,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Device in courtyard: '%1' ↔ '%2'",
           "Placeholders: Device 1 name, device 2 name")
            .arg(std::min(*device1.getComponentInstance().getName(),
                          *device2.getComponentInstance().getName()),
                 std::max(*device1.getComponentInstance().getName(),
                          *device2.getComponentInstance().getName())),
        tr("A device is placed within the courtyard of another device, which "
           "might cause troubles during assembly of these parts.") %
            "\n\n" %
            tr("Either move the devices to increase their clearance or approve "
               "this message if you're sure they can be assembled without "
               "problems."),
        "device_in_courtyard", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device",
                         std::min(device1.getComponentInstanceUuid(),
                                  device2.getComponentInstanceUuid()));
  mApproval->ensureLineBreak();
  mApproval->appendChild("device",
                         std::max(device1.getComponentInstanceUuid(),
                                  device2.getComponentInstanceUuid()));
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgOverlappingDevices
 ******************************************************************************/

DrcMsgOverlappingDevices::DrcMsgOverlappingDevices(
    const BI_Device& device1, const BI_Device& device2,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Device overlap: '%1' ↔ '%2'",
           "Placeholders: Device 1 name, device 2 name")
            .arg(std::min(*device1.getComponentInstance().getName(),
                          *device2.getComponentInstance().getName()),
                 std::max(*device1.getComponentInstance().getName(),
                          *device2.getComponentInstance().getName())),
        tr("Two devices are overlapping and thus probably cannot be assembled "
           "both at the same time.") %
            "\n\n" %
            tr("Either move the devices to increase their clearance or approve "
               "this message if you're sure they can be assembled without "
               "problems (or only one of them gets assembled)."),
        "overlapping_devices", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device",
                         std::min(device1.getComponentInstanceUuid(),
                                  device2.getComponentInstanceUuid()));
  mApproval->ensureLineBreak();
  mApproval->appendChild("device",
                         std::max(device1.getComponentInstanceUuid(),
                                  device2.getComponentInstanceUuid()));
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgExposureInKeepoutZone
 ******************************************************************************/

DrcMsgDeviceInKeepoutZone::DrcMsgDeviceInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Device& device,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Device in keepout zone: '%1'", "Placeholder is device name")
            .arg(*device.getComponentInstance().getName()),
        getDescription(), "device_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

void DrcMsgDeviceInKeepoutZone::addZoneApprovalNodes(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone) noexcept {
  if (boardZone) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("zone", boardZone->getData().getUuid());
  }
  if (deviceZone) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("zone", deviceZone->getUuid());
  }
  if (zoneDevice) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("from_device",
                           zoneDevice->getComponentInstanceUuid());
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
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_FootprintPad& pad,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pad in exposure keepout zone: '%1'", "Placeholder is pad name")
            .arg(pad.getText()),
        getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Via& via,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Via in exposure keepout zone: '%1'", "Placeholder is net name")
            .arg(via.getNetSegment().getNetNameToDisplay(true)),
        getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Polygon& polygon,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, tr("Polygon in exposure keepout zone"),
                     getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("polygon", polygon.getData().getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Device& device, const Polygon& polygon,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error,
                     tr("Polygon in exposure keepout zone: '%1'",
                        "Placeholder is device name")
                         .arg(*device.getComponentInstance().getName()),
                     getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("polygon", polygon.getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

DrcMsgExposureInKeepoutZone::DrcMsgExposureInKeepoutZone(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone, const BI_Device& device, const Circle& circle,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error,
                     tr("Circle in exposure keepout zone: '%1'",
                        "Placeholder is device name")
                         .arg(*device.getComponentInstance().getName()),
                     getDescription(), "exposure_in_keepout_zone", locations) {
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("circle", circle.getUuid());
  mApproval->ensureLineBreak();
  addZoneApprovalNodes(boardZone, zoneDevice, deviceZone);
  mApproval->ensureLineBreak();
}

void DrcMsgExposureInKeepoutZone::addZoneApprovalNodes(
    const BI_Zone* boardZone, const BI_Device* zoneDevice,
    const Zone* deviceZone) noexcept {
  if (boardZone) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("zone", boardZone->getData().getUuid());
  }
  if (deviceZone) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("zone", deviceZone->getUuid());
  }
  if (zoneDevice) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("from_device",
                           zoneDevice->getComponentInstanceUuid());
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
    const BI_Via& via, const UnsignedLength& minAnnularWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Via annular ring of '%1' < %2 %3",
           "Placeholders: Net name, minimum annular width, unit")
            .arg(via.getNetSegment().getNetNameToDisplay(true),
                 minAnnularWidth->toMmString(), "mm"),
        tr("The via annular ring width (i.e. the copper around the hole) is "
           "smaller than the minimum annular width configured in the DRC "
           "settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the via size if needed."),
        "minimum_annular_ring_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumAnnularRingViolation::DrcMsgMinimumAnnularRingViolation(
    const BI_FootprintPad& pad, const UnsignedLength& minAnnularWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pad annular ring of '%1' < %2 %3",
           "Placeholders: Net name, minimum annular width, unit")
            .arg(pad.getText(), minAnnularWidth->toMmString(), "mm"),
        tr("The through-hole pad annular ring width (i.e. the copper around "
           "the hole) is smaller than the minimum annular width configured in "
           "the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the pad size if needed."),
        "minimum_annular_ring_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumDrillDiameterViolation
 ******************************************************************************/

DrcMsgMinimumDrillDiameterViolation::DrcMsgMinimumDrillDiameterViolation(
    const BI_Hole& hole, const UnsignedLength& minDiameter,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        determineMessage(hole.getData().getDiameter(), minDiameter),
        determineDescription(false, false), "minimum_drill_diameter_violation",
        locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumDrillDiameterViolation::DrcMsgMinimumDrillDiameterViolation(
    const BI_Device& device, const Hole& hole,
    const UnsignedLength& minDiameter, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     determineMessage(hole.getDiameter(), minDiameter),
                     determineDescription(false, false),
                     "minimum_drill_diameter_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumDrillDiameterViolation::DrcMsgMinimumDrillDiameterViolation(
    const BI_Via& via, const UnsignedLength& minDiameter,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Via drill diameter of '%1': %2 < %3 %4",
           "Placeholders: Net name, actual diameter, minimum diameter")
            .arg(via.getNetSegment().getNetNameToDisplay(true),
                 via.getDrillDiameter()->toMmString(),
                 minDiameter->toMmString(), "mm"),
        determineDescription(true, false), "minimum_drill_diameter_violation",
        locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumDrillDiameterViolation::DrcMsgMinimumDrillDiameterViolation(
    const BI_FootprintPad& pad, const PadHole& padHole,
    const UnsignedLength& minDiameter, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Pad drill diameter of '%1': %2 < %3 %4",
           "Placeholders: Net name, actual diameter, minimum diameter")
            .arg(pad.getText(), padHole.getDiameter()->toMmString(),
                 minDiameter->toMmString(), "mm"),
        determineDescription(false, true), "minimum_drill_diameter_violation",
        locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", padHole.getUuid());
  mApproval->ensureLineBreak();
}

QString DrcMsgMinimumDrillDiameterViolation::determineMessage(
    const PositiveLength& actualDiameter,
    const UnsignedLength& minDiameter) noexcept {
  return tr("NPTH drill diameter: %1 < %2 %3",
            "Placeholders: Actual diameter, minimum diameter, unit")
      .arg(actualDiameter->toMmString(), minDiameter->toMmString(), "mm");
}

QString DrcMsgMinimumDrillDiameterViolation::determineDescription(
    bool isVia, bool isPad) noexcept {
  QString s;
  if (isVia) {
    s +=
        tr("The drill diameter of the via is smaller than the minimum plated "
           "drill diameter configured in the DRC settings.");
  } else if (isPad) {
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
    const BI_Hole& hole, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     determineMessage(hole.getData().getDiameter(), minWidth),
                     determineDescription(false),
                     "minimum_slot_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumSlotWidthViolation::DrcMsgMinimumSlotWidthViolation(
    const BI_Device& device, const Hole& hole, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     determineMessage(hole.getDiameter(), minWidth),
                     determineDescription(false),
                     "minimum_slot_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgMinimumSlotWidthViolation::DrcMsgMinimumSlotWidthViolation(
    const BI_FootprintPad& pad, const PadHole& padHole,
    const UnsignedLength& minWidth, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Pad slot width of '%1': %2 < %3 %4",
           "Placeholders: Net name, actual width, minimum width, unit")
            .arg(pad.getText(), padHole.getDiameter()->toMmString(),
                 minWidth->toMmString(), "mm"),
        determineDescription(true), "minimum_slot_width_violation", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", padHole.getUuid());
  mApproval->ensureLineBreak();
}

QString DrcMsgMinimumSlotWidthViolation::determineMessage(
    const PositiveLength& actualWidth,
    const UnsignedLength& minWidth) noexcept {
  return tr("NPTH slot width: %1 < %2 %3",
            "Placeholders: Actual width, minimum width, unit")
      .arg(actualWidth->toMmString(), minWidth->toMmString(), "mm");
}

QString DrcMsgMinimumSlotWidthViolation::determineDescription(
    bool isPad) noexcept {
  QString s;
  if (isPad) {
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
    const BI_FootprintPad& pad, const Layer& layer,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Invalid connection of pad '%1' on '%2'",
           "Placeholders: Pad name, layer name")
            .arg(pad.getText()),
        tr("The pad origin must be located within the pads copper area, "
           "or for THT pads within a hole. Otherwise traces might not be"
           "connected fully. This issue needs to be fixed in the "
           "library."),
        "invalid_pad_connection", locations) {
  mApproval->appendChild("layer", layer);
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgForbiddenSlotUsed
 ******************************************************************************/

DrcMsgForbiddenSlot::DrcMsgForbiddenSlot(
    const BI_Hole& hole, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     determineMessage(hole.getData().getPath()),
                     determineDescription(hole.getData().getPath()),
                     "forbidden_slot", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getData().getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgForbiddenSlot::DrcMsgForbiddenSlot(
    const BI_Device& device, const Hole& hole,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, determineMessage(hole.getPath()),
                     determineDescription(hole.getPath()), "forbidden_slot",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", device.getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", hole.getUuid());
  mApproval->ensureLineBreak();
}

DrcMsgForbiddenSlot::DrcMsgForbiddenSlot(
    const BI_FootprintPad& pad, const PadHole& padHole,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, determineMessage(padHole.getPath()),
                     determineDescription(padHole.getPath()), "forbidden_slot",
                     locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pad", pad.getLibPadUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("hole", padHole.getUuid());
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

DrcMsgForbiddenVia::DrcMsgForbiddenVia(const BI_Via& via,
                                       const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Error, determineMessage(via),
                     determineDescription(via), "forbidden_via", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
  mApproval->ensureLineBreak();
}

QString DrcMsgForbiddenVia::determineMessage(const BI_Via& via) noexcept {
  const QString net = via.getNetSegment().getNetNameToDisplay(true);
  if (via.getVia().isBlind()) {
    return tr("Blind via in net '%1'").arg(net);
  } else {
    return tr("Buried via in net '%1'").arg(net);
  }
}

QString DrcMsgForbiddenVia::determineDescription(const BI_Via& via) noexcept {
  const QString suggestion = "\n" %
      tr("Either avoid them or check if your PCB manufacturer supports them "
         "and adjust the DRC settings accordingly.");
  if (via.getVia().isBlind()) {
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
 *  Class DrcMsgSilkscreenClearanceViolation
 ******************************************************************************/

DrcMsgSilkscreenClearanceViolation::DrcMsgSilkscreenClearanceViolation(
    const BI_StrokeText& strokeText, const UnsignedLength& minClearance,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance silkscreen text ↔ stop mask < %1 %2",
           "Placeholders: Clearance value, unit")
            .arg(minClearance->toMmString(), "mm"),
        tr("The clearance between a silkscreen text and a solder resist "
           "opening "
           "is smaller than the minimum clearance configured in the DRC "
           "settings. This could lead to clipped silkscreen during "
           "production.") %
            "\n\n" %
            tr("Check the DRC settings and move the text away from the "
               "solder resist opening if needed."),
        "silkscreen_clearance_violation", locations) {
  mApproval->ensureLineBreak();
  if (const BI_Device* device = strokeText.getDevice()) {
    mApproval->appendChild("device", device->getComponentInstanceUuid());
    mApproval->ensureLineBreak();
  }
  mApproval->appendChild("stroke_text", strokeText.getData().getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgUselessZone
 ******************************************************************************/

DrcMsgUselessZone::DrcMsgUselessZone(const BI_Zone& zone,
                                     const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Useless zone"),
        tr("The zone has no layer or rule enabled so it is useless."),
        "useless_zone", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("zone", zone.getData().getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgUselessVia
 ******************************************************************************/

DrcMsgUselessVia::DrcMsgUselessVia(const BI_Via& via,
                                   const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     tr("Useless via in net '%1'", "Placeholders: Net name")
                         .arg(via.getNetSegment().getNetNameToDisplay(true)),
                     tr("The via is connected on less than two layers, thus it "
                        "seems to be useless."),
                     "useless_via", locations) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("via", via.getUuid());
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
