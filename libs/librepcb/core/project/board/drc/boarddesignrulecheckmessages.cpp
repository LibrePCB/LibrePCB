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
#include "../../../geometry/polygon.h"
#include "../../../graphics/graphicslayer.h"
#include "../../../library/pkg/packagepad.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/netsignal.h"
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", component.getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  SExpression& fromNode = mApproval.appendList("tmp");
  mApproval.ensureLineBreak();
  SExpression& toNode = mApproval.appendList("tmp");
  mApproval.ensureLineBreak();

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
  mApproval.ensureLineBreak();
  mApproval.appendChild("netsegment", netSegment.getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("board", netPoint.getBoard().getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("netsegment", netPoint.getNetSegment().getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("junction", netPoint.getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("netsegment", netLine.getNetSegment().getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("trace", netLine.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_Plane& plane, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Min. plane width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(GraphicsLayer::getTranslation(*plane.getLayerName()),
                 plane.getMinWidth()->toMmString(), minWidth->toMmString(),
                 "mm"),
        tr("The configured minimum width of the plane is smaller than the "
           "minimum copper width configured in the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the minimum plane width in "
               "its properties if needed."),
        "minimum_width_violation", locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("plane", plane.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgMinimumWidthViolation::DrcMsgMinimumWidthViolation(
    const BI_StrokeText& text, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Stroke width on '%1': %2 < %3 %4",
           "Placeholders: Layer name, actual width, minimum width, unit")
            .arg(GraphicsLayer::getTranslation(*text.getText().getLayerName()),
                 text.getText().getStrokeWidth()->toMmString(),
                 minWidth->toMmString(), "mm"),
        tr("The text stroke width is smaller than the minimum copper width "
           "configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings and increase the text stroke width if "
               "needed."),
        "minimum_width_violation", locations) {
  mApproval.ensureLineBreak();
  if (const BI_Device* device = text.getDevice()) {
    mApproval.appendChild("device", device->getComponentInstanceUuid());
    mApproval.ensureLineBreak();
  }
  mApproval.appendChild("stroke_text", text.getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgCopperCopperClearanceViolation
 ******************************************************************************/

DrcMsgCopperCopperClearanceViolation::DrcMsgCopperCopperClearanceViolation(
    const QString& layer1, const NetSignal* net1, const BI_Base& item1,
    const Polygon* polygon1, const Circle* circle1, const QString& layer2,
    const NetSignal* net2, const BI_Base& item2, const Polygon* polygon2,
    const Circle* circle2, const UnsignedLength& minClearance,
    const QVector<Path>& locations)
  : RuleCheckMessage(
        Severity::Error,
        tr("Clearance on %1: %2 ↔ %3 < %4 %5",
           "Placeholders: Layer name, object name, object name, "
           "Clearance value, unit")
            .arg(getLayerName(layer1, layer2),
                 getObjectName(net1, item1, polygon1, circle1),
                 getObjectName(net2, item2, polygon2, circle2),
                 minClearance->toMmString(), "mm"),
        tr("The clearance between two copper objects of different nets is "
           "smaller than the minimum copper clearance configured in the DRC "
           "settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and move the objects to increase their "
               "clearance if needed."),
        "copper_clearance_violation", locations) {
  mApproval.ensureLineBreak();
  SExpression& node1 = mApproval.appendList("object");
  mApproval.ensureLineBreak();
  SExpression& node2 = mApproval.appendList("object");
  mApproval.ensureLineBreak();

  // Sort nodes to make the approval canonical.
  serializeObject(node1, item1, polygon1, circle1);
  serializeObject(node2, item2, polygon2, circle2);
  if (node2 < node1) {
    std::swap(node1, node2);
  }
}

QString DrcMsgCopperCopperClearanceViolation::getLayerName(
    const QString& layer1, const QString& layer2) {
  if (!layer1.isEmpty()) {
    return "'" % GraphicsLayer::getTranslation(layer1) % "'";
  } else if (!layer2.isEmpty()) {
    return "'" % GraphicsLayer::getTranslation(layer2) % "'";
  } else {
    return tr("copper layers");
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
    name = QString("'%1:%2'").arg(
        *pad->getDevice().getComponentInstance().getName(),
        *pad->getLibPackagePad()->getName());
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
    node.appendChild("polygon", polygon->getUuid());
  } else if (const BI_StrokeText* strokeText =
                 dynamic_cast<const BI_StrokeText*>(&item)) {
    if (const BI_Device* device = strokeText->getDevice()) {
      node.ensureLineBreak();
      node.appendChild("device", device->getComponentInstanceUuid());
      node.ensureLineBreak();
      node.appendChild("stroke_text", strokeText->getUuid());
      node.ensureLineBreak();
    } else {
      node.appendChild("stroke_text", strokeText->getUuid());
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("via", via.getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("netsegment", netLine.getNetSegment().getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("trace", netLine.getUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad.getLibPadUuid());
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("plane", plane.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Device* device, const Polygon& polygon,
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
  mApproval.ensureLineBreak();
  if (device) {
    mApproval.appendChild("device", device->getComponentInstanceUuid());
    mApproval.ensureLineBreak();
  }
  mApproval.appendChild("polygon", polygon.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Device* device, const Circle& circle,
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
  mApproval.ensureLineBreak();
  if (device) {
    mApproval.appendChild("device", device->getComponentInstanceUuid());
    mApproval.ensureLineBreak();
  }
  mApproval.appendChild("circle", circle.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgCopperBoardClearanceViolation::DrcMsgCopperBoardClearanceViolation(
    const BI_Device* device, const StrokeText& strokeText,
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
  mApproval.ensureLineBreak();
  if (device) {
    mApproval.appendChild("device", device->getComponentInstanceUuid());
    mApproval.ensureLineBreak();
  }
  mApproval.appendChild("stroke_text", strokeText.getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgCopperHoleClearanceViolation
 ******************************************************************************/

DrcMsgCopperHoleClearanceViolation::DrcMsgCopperHoleClearanceViolation(
    const BI_Device* device, const Hole& hole,
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
  mApproval.ensureLineBreak();
  if (device) {
    mApproval.appendChild("device", device->getComponentInstanceUuid());
    mApproval.ensureLineBreak();
  }
  mApproval.appendChild("hole", hole.getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgCourtyardOverlap
 ******************************************************************************/

DrcMsgCourtyardOverlap::DrcMsgCourtyardOverlap(
    const BI_Device& device1, const BI_Device& device2,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Courtyard overlap: '%1' ↔ '%2'",
           "Placeholders: Device 1 name, device 2 name")
            .arg(std::min(*device1.getComponentInstance().getName(),
                          *device2.getComponentInstance().getName()),
                 std::max(*device1.getComponentInstance().getName(),
                          *device2.getComponentInstance().getName())),
        tr("The courtyard of two devices overlap, which might cause troubles "
           "during assembly of these parts.") %
            "\n\n" %
            tr("Either move the devices to increase their distance or approve "
               "this message if you're sure they can be assembled without "
               "problems."),
        "courtyard_overlap", locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("device",
                        std::min(device1.getComponentInstanceUuid(),
                                 device2.getComponentInstanceUuid()));
  mApproval.ensureLineBreak();
  mApproval.appendChild("device",
                        std::max(device1.getComponentInstanceUuid(),
                                 device2.getComponentInstanceUuid()));
  mApproval.ensureLineBreak();
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
  mApproval.ensureLineBreak();
  mApproval.appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("via", via.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgMinimumAnnularRingViolation::DrcMsgMinimumAnnularRingViolation(
    const BI_FootprintPad& pad, const UnsignedLength& minAnnularWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pad annular ring of '%1' < %2 %3",
           "Placeholders: Net name, minimum annular width, unit")
            .arg(pad.getDisplayText().simplified(),
                 minAnnularWidth->toMmString(), "mm"),
        tr("The through-hole pad annular ring width (i.e. the copper around "
           "the hole) is smaller than the minimum annular width configured in "
           "the DRC settings.") %
            " " % seriousTroublesTr() % "\n\n" %
            tr("Check the DRC settings and increase the pad size if needed."),
        "minimum_annular_ring_violation", locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad.getLibPadUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumDrillDiameterViolation
 ******************************************************************************/

DrcMsgMinimumDrillDiameterViolation::DrcMsgMinimumDrillDiameterViolation(
    const BI_Device* device, const Hole& hole,
    const UnsignedLength& minDiameter, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("NPTH drill diameter: %1 < %2 %3",
           "Placeholders: Actual diameter, minimum diameter, unit")
            .arg(hole.getDiameter()->toMmString(), minDiameter->toMmString(),
                 "mm"),
        tr("The drill diameter of the non-plated hole is smaller than the "
           "minimum non-plated drill diameter configured in the DRC "
           "settings.") %
            "\n\n" %
            tr("Check the DRC settings or increase the drill diameter if "
               "needed."),
        "minimum_drill_diameter_violation", locations) {
  mApproval.ensureLineBreak();
  if (device) {
    mApproval.appendChild("device", device->getComponentInstanceUuid());
    mApproval.ensureLineBreak();
  }
  mApproval.appendChild("hole", hole.getUuid());
  mApproval.ensureLineBreak();
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
        tr("The drill diameter of the via is smaller than the "
           "minimum plated drill diameter configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings or increase the drill diameter if "
               "needed."),
        "minimum_drill_diameter_violation", locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("netsegment", via.getNetSegment().getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("via", via.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgMinimumDrillDiameterViolation::DrcMsgMinimumDrillDiameterViolation(
    const BI_FootprintPad& pad, const PadHole& padHole,
    const UnsignedLength& minDiameter, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Pad drill diameter of '%1': %2 < %3 %4",
           "Placeholders: Net name, actual diameter, minimum diameter")
            .arg(pad.getDisplayText().simplified(),
                 padHole.getDiameter()->toMmString(), minDiameter->toMmString(),
                 "mm"),
        tr("The drill diameter of the through-hole pad is smaller than the "
           "minimum plated drill diameter configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings or increase the drill diameter if "
               "needed."),
        "minimum_drill_diameter_violation", locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad.getLibPadUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("hole", padHole.getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgMinimumSlotWidthViolation
 ******************************************************************************/

DrcMsgMinimumSlotWidthViolation::DrcMsgMinimumSlotWidthViolation(
    const BI_Device* device, const Hole& hole, const UnsignedLength& minWidth,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("NPTH slot width: %1 < %2 %3",
           "Placeholders: Actual width, minimum width, unit")
            .arg(hole.getDiameter()->toMmString(), minWidth->toMmString(),
                 "mm"),
        tr("The width of the non-plated slot is smaller than the "
           "minimum non-plated slot width configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings or increase the slot width if "
               "needed."),
        "minimum_slot_width_violation", locations) {
  mApproval.ensureLineBreak();
  if (device) {
    mApproval.appendChild("device", device->getComponentInstanceUuid());
    mApproval.ensureLineBreak();
  }
  mApproval.appendChild("hole", hole.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgMinimumSlotWidthViolation::DrcMsgMinimumSlotWidthViolation(
    const BI_FootprintPad& pad, const PadHole& padHole,
    const UnsignedLength& minWidth, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Pad slot width of '%1': %2 < %3 %4",
           "Placeholders: Net name, actual width, minimum width, unit")
            .arg(pad.getDisplayText().simplified(),
                 padHole.getDiameter()->toMmString(), minWidth->toMmString(),
                 "mm"),
        tr("The width of the plated slot is smaller than the minimum plated "
           "slot width configured in the DRC settings.") %
            "\n\n" %
            tr("Check the DRC settings or increase the slot width if needed."),
        "minimum_slot_width_violation", locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad.getLibPadUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("hole", padHole.getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgInvalidPadConnection
 ******************************************************************************/

DrcMsgInvalidPadConnection::DrcMsgInvalidPadConnection(
    const BI_FootprintPad& pad, const GraphicsLayer& layer,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Invalid connection of pad '%1' on '%2'",
           "Placeholders: Pad name, layer name")
            .arg(pad.getDisplayText().simplified()),
        tr("The pad origin must be located within the pads copper area, "
           "or for THT pads within a hole. Otherwise traces might not be"
           "connected fully. This issue needs to be fixed in the "
           "library."),
        "invalid_pad_connection", locations) {
  mApproval.appendChild("layer", SExpression::createToken(layer.getName()));
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad.getLibPadUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  DrcMsgForbiddenSlotUsed
 ******************************************************************************/

DrcMsgForbiddenSlot::DrcMsgForbiddenSlot(
    const BI_Hole& hole, const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning,
                     determineMessage(hole.getHole().getPath()),
                     determineDescription(hole.getHole().getPath()),
                     "forbidden_slot", locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("hole", hole.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgForbiddenSlot::DrcMsgForbiddenSlot(
    const BI_Device& device, const Hole& hole,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, determineMessage(hole.getPath()),
                     determineDescription(hole.getPath()), "forbidden_slot",
                     locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", device.getComponentInstanceUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("hole", hole.getUuid());
  mApproval.ensureLineBreak();
}

DrcMsgForbiddenSlot::DrcMsgForbiddenSlot(
    const BI_FootprintPad& pad, const PadHole& padHole,
    const QVector<Path>& locations) noexcept
  : RuleCheckMessage(Severity::Warning, determineMessage(padHole.getPath()),
                     determineDescription(padHole.getPath()), "forbidden_slot",
                     locations) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("device", pad.getDevice().getComponentInstanceUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad.getLibPadUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("hole", padHole.getUuid());
  mApproval.ensureLineBreak();
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
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
