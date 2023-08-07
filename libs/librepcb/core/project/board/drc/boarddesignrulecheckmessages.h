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

#ifndef LIBREPCB_CORE_BOARDDESIGNRULECHECKMESSAGES_H
#define LIBREPCB_CORE_BOARDDESIGNRULECHECKMESSAGES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../rulecheck/rulecheckmessage.h"
#include "../../../types/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Base;
class BI_Device;
class BI_FootprintPad;
class BI_Hole;
class BI_NetLine;
class BI_NetLineAnchor;
class BI_NetPoint;
class BI_NetSegment;
class BI_Plane;
class BI_Polygon;
class BI_StrokeText;
class BI_Via;
class BI_Zone;
class Circle;
class ComponentInstance;
class Hole;
class Layer;
class NetSignal;
class PadHole;
class Polygon;
class StrokeText;
class Uuid;
class Zone;

/*******************************************************************************
 *  Class DrcMsgMissingDevice
 ******************************************************************************/

/**
 * @brief The DrcMsgMissingDevice class
 */
class DrcMsgMissingDevice final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMissingDevice)

public:
  // Constructors / Destructor
  DrcMsgMissingDevice() = delete;
  explicit DrcMsgMissingDevice(const ComponentInstance& component) noexcept;
  DrcMsgMissingDevice(const DrcMsgMissingDevice& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMissingDevice() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgMissingConnection
 ******************************************************************************/

/**
 * @brief The DrcMsgMissingConnection class
 */
class DrcMsgMissingConnection final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMissingConnection)

public:
  // Constructors / Destructor
  DrcMsgMissingConnection() = delete;
  DrcMsgMissingConnection(const BI_NetLineAnchor& p1,
                          const BI_NetLineAnchor& p2,
                          const NetSignal& netSignal,
                          const QVector<Path>& locations);
  DrcMsgMissingConnection(const DrcMsgMissingConnection& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMissingConnection() noexcept {}

private:
  static QString getAnchorName(const BI_NetLineAnchor& anchor);
  static void serializeAnchor(SExpression& node,
                              const BI_NetLineAnchor& anchor);
};

/*******************************************************************************
 *  Class DrcMsgMissingBoardOutline
 ******************************************************************************/

/**
 * @brief The DrcMsgMissingBoardOutline class
 */
class DrcMsgMissingBoardOutline final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMissingBoardOutline)

public:
  // Constructors / Destructor
  DrcMsgMissingBoardOutline() noexcept;
  DrcMsgMissingBoardOutline(const DrcMsgMissingBoardOutline& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMissingBoardOutline() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgMultipleBoardOutlines
 ******************************************************************************/

/**
 * @brief The DrcMsgMultipleBoardOutlines class
 */
class DrcMsgMultipleBoardOutlines final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMultipleBoardOutlines)

public:
  // Constructors / Destructor
  explicit DrcMsgMultipleBoardOutlines(const QVector<Path>& locations) noexcept;
  DrcMsgMultipleBoardOutlines(const DrcMsgMultipleBoardOutlines& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMultipleBoardOutlines() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgOpenBoardOutlinePolygon
 ******************************************************************************/

/**
 * @brief The DrcMsgOpenBoardOutlinePolygon class
 */
class DrcMsgOpenBoardOutlinePolygon final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgOpenBoardOutlinePolygon)

public:
  // Constructors / Destructor
  DrcMsgOpenBoardOutlinePolygon() = delete;
  DrcMsgOpenBoardOutlinePolygon(const BI_Device* device, const Uuid& polygon,
                                const QVector<Path>& locations) noexcept;
  DrcMsgOpenBoardOutlinePolygon(
      const DrcMsgOpenBoardOutlinePolygon& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgOpenBoardOutlinePolygon() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgMinimumBoardOutlineInnerRadiusViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgMinimumBoardOutlineInnerRadiusViolation class
 */
class DrcMsgMinimumBoardOutlineInnerRadiusViolation final
  : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMinimumBoardOutlineInnerRadiusViolation)

public:
  // Constructors / Destructor
  DrcMsgMinimumBoardOutlineInnerRadiusViolation() = delete;
  DrcMsgMinimumBoardOutlineInnerRadiusViolation(
      const UnsignedLength& minRadius, const QVector<Path>& locations) noexcept;
  DrcMsgMinimumBoardOutlineInnerRadiusViolation(
      const DrcMsgMinimumBoardOutlineInnerRadiusViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumBoardOutlineInnerRadiusViolation() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgEmptyNetSegment
 ******************************************************************************/

/**
 * @brief The DrcMsgEmptyNetSegment class
 */
class DrcMsgEmptyNetSegment final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgEmptyNetSegment)

public:
  // Constructors / Destructor
  DrcMsgEmptyNetSegment() = delete;
  explicit DrcMsgEmptyNetSegment(const BI_NetSegment& netSegment) noexcept;
  DrcMsgEmptyNetSegment(const DrcMsgEmptyNetSegment& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgEmptyNetSegment() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgUnconnectedJunction
 ******************************************************************************/

/**
 * @brief The DrcMsgUnconnectedJunction class
 */
class DrcMsgUnconnectedJunction final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgUnconnectedJunction)

public:
  // Constructors / Destructor
  DrcMsgUnconnectedJunction() = delete;
  DrcMsgUnconnectedJunction(const BI_NetPoint& netPoint,
                            const QVector<Path>& locations) noexcept;
  DrcMsgUnconnectedJunction(const DrcMsgUnconnectedJunction& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgUnconnectedJunction() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgMinimumTextHeightViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgMinimumTextHeightViolation class
 */
class DrcMsgMinimumTextHeightViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMinimumTextHeightViolation)

public:
  // Constructors / Destructor
  DrcMsgMinimumTextHeightViolation() = delete;
  DrcMsgMinimumTextHeightViolation(const BI_StrokeText& text,
                                   const UnsignedLength& minHeight,
                                   const QVector<Path>& locations) noexcept;
  DrcMsgMinimumTextHeightViolation(
      const DrcMsgMinimumTextHeightViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumTextHeightViolation() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgMinimumWidthViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgMinimumWidthViolation class
 */
class DrcMsgMinimumWidthViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMinimumWidthViolation)

public:
  // Constructors / Destructor
  DrcMsgMinimumWidthViolation() = delete;
  DrcMsgMinimumWidthViolation(const BI_NetLine& netLine,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const BI_Plane& plane,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const BI_Polygon& polygon,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const BI_StrokeText& text,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const BI_Device& device, const Polygon& polygon,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const BI_Device& device, const Circle& circle,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const DrcMsgMinimumWidthViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumWidthViolation() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgCopperCopperClearanceViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgCopperCopperClearanceViolation class
 */
class DrcMsgCopperCopperClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgCopperCopperClearanceViolation)

public:
  // Constructors / Destructor
  DrcMsgCopperCopperClearanceViolation() = delete;
  DrcMsgCopperCopperClearanceViolation(
      const NetSignal* net1, const BI_Base& item1, const Polygon* polygon1,
      const Circle* circle1, const NetSignal* net2, const BI_Base& item2,
      const Polygon* polygon2, const Circle* circle2,
      const QVector<const Layer*>& layers, const Length& minClearance,
      const QVector<Path>& locations);
  DrcMsgCopperCopperClearanceViolation(
      const DrcMsgCopperCopperClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperCopperClearanceViolation() noexcept {}

private:
  static QString getLayerName(const QVector<const Layer*>& layers);
  static QString getObjectName(const NetSignal* net, const BI_Base& item,
                               const Polygon* polygon, const Circle* circle);
  static void serializeObject(SExpression& node, const BI_Base& item,
                              const Polygon* polygon, const Circle* circle);
};

/*******************************************************************************
 *  Class DrcMsgCopperBoardClearanceViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgCopperBoardClearanceViolation class
 */
class DrcMsgCopperBoardClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgCopperBoardClearanceViolation)

public:
  // Constructors / Destructor
  DrcMsgCopperBoardClearanceViolation() = delete;
  DrcMsgCopperBoardClearanceViolation(const BI_Via& via,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const BI_NetLine& netLine,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const BI_FootprintPad& pad,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const BI_Plane& plane,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const BI_Polygon& polygon,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const BI_Device& device,
                                      const Polygon& polygon,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const BI_Device& device,
                                      const Circle& circle,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const BI_StrokeText& strokeText,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(
      const DrcMsgCopperBoardClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperBoardClearanceViolation() noexcept {}

private:
  static QString getPolygonMessage(const UnsignedLength& minClearance) noexcept;
  static QString getPolygonDescription() noexcept;
};

/*******************************************************************************
 *  Class DrcMsgCopperHoleClearanceViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgCopperHoleClearanceViolation class
 */
class DrcMsgCopperHoleClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgCopperHoleClearanceViolation)

public:
  // Constructors / Destructor
  DrcMsgCopperHoleClearanceViolation() = delete;
  DrcMsgCopperHoleClearanceViolation(const BI_Hole& hole,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgCopperHoleClearanceViolation(const BI_Device& device, const Hole& hole,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgCopperHoleClearanceViolation(
      const DrcMsgCopperHoleClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperHoleClearanceViolation() noexcept {}

private:
  static QString getMessage(const UnsignedLength& minClearance) noexcept;
  static QString getDescription() noexcept;
};

/*******************************************************************************
 *  Class DrcMsgCopperInKeepoutZone
 ******************************************************************************/

/**
 * @brief The DrcMsgCopperInKeepoutZone class
 */
class DrcMsgCopperInKeepoutZone final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgCopperInKeepoutZone)

public:
  // Constructors / Destructor
  DrcMsgCopperInKeepoutZone() = delete;
  DrcMsgCopperInKeepoutZone(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice, const Zone* deviceZone,
                            const BI_FootprintPad& pad,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice, const Zone* deviceZone,
                            const BI_Via& via,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice, const Zone* deviceZone,
                            const BI_NetLine& netLine,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice, const Zone* deviceZone,
                            const BI_Polygon& polygon,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice, const Zone* deviceZone,
                            const BI_Device& device, const Polygon& polygon,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice, const Zone* deviceZone,
                            const BI_Device& device, const Circle& circle,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const DrcMsgCopperInKeepoutZone& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperInKeepoutZone() noexcept {}

private:
  void addZoneApprovalNodes(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice,
                            const Zone* deviceZone) noexcept;
  static QString getDescription() noexcept;
};

/*******************************************************************************
 *  Class DrcMsgDrillDrillClearanceViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgDrillDrillClearanceViolation class
 */
class DrcMsgDrillDrillClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgDrillDrillClearanceViolation)

public:
  // Constructors / Destructor
  DrcMsgDrillDrillClearanceViolation() = delete;
  DrcMsgDrillDrillClearanceViolation(const BI_Base& item1, const Uuid& hole1,
                                     const BI_Base& item2, const Uuid& hole2,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations);
  DrcMsgDrillDrillClearanceViolation(
      const DrcMsgDrillDrillClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDrillDrillClearanceViolation() noexcept {}

private:  // Methods
  static void serializeObject(SExpression& node, const BI_Base& item,
                              const Uuid& hole);
};

/*******************************************************************************
 *  Class DrcMsgDrillBoardClearanceViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgDrillBoardClearanceViolation class
 */
class DrcMsgDrillBoardClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgDrillBoardClearanceViolation)

public:
  // Constructors / Destructor
  DrcMsgDrillBoardClearanceViolation() = delete;
  DrcMsgDrillBoardClearanceViolation(const BI_Via& via,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgDrillBoardClearanceViolation(const BI_FootprintPad& pad,
                                     const PadHole& hole,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgDrillBoardClearanceViolation(const BI_Hole& hole,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgDrillBoardClearanceViolation(const BI_Device& device, const Hole& hole,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgDrillBoardClearanceViolation(
      const DrcMsgDrillBoardClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDrillBoardClearanceViolation() noexcept {}

private:  // Methods
  static QString getMessage(const UnsignedLength& minClearance) noexcept;
  static QString getDescription() noexcept;
};

/*******************************************************************************
 *  Class DrcMsgCourtyardOverlap
 ******************************************************************************/

/**
 * @brief The DrcMsgCourtyardOverlap class
 */
class DrcMsgCourtyardOverlap final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgCourtyardOverlap)

public:
  // Constructors / Destructor
  DrcMsgCourtyardOverlap() = delete;
  DrcMsgCourtyardOverlap(const BI_Device& device1, const BI_Device& device2,
                         const QVector<Path>& locations) noexcept;
  DrcMsgCourtyardOverlap(const DrcMsgCourtyardOverlap& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCourtyardOverlap() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgDeviceInKeepoutZone
 ******************************************************************************/

/**
 * @brief The DrcMsgDeviceInKeepoutZone class
 */
class DrcMsgDeviceInKeepoutZone final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgDeviceInKeepoutZone)

public:
  // Constructors / Destructor
  DrcMsgDeviceInKeepoutZone() = delete;
  DrcMsgDeviceInKeepoutZone(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice, const Zone* deviceZone,
                            const BI_Device& device,
                            const QVector<Path>& locations) noexcept;
  DrcMsgDeviceInKeepoutZone(const DrcMsgDeviceInKeepoutZone& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDeviceInKeepoutZone() noexcept {}

private:
  void addZoneApprovalNodes(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice,
                            const Zone* deviceZone) noexcept;
  static QString getDescription() noexcept;
};

/*******************************************************************************
 *  Class DrcMsgExposureInKeepoutZone
 ******************************************************************************/

/**
 * @brief The DrcMsgExposureInKeepoutZone class
 */
class DrcMsgExposureInKeepoutZone final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgExposureInKeepoutZone)

public:
  // Constructors / Destructor
  DrcMsgExposureInKeepoutZone() = delete;
  DrcMsgExposureInKeepoutZone(const BI_Zone* boardZone,
                              const BI_Device* zoneDevice,
                              const Zone* deviceZone,
                              const BI_FootprintPad& pad,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const BI_Zone* boardZone,
                              const BI_Device* zoneDevice,
                              const Zone* deviceZone, const BI_Via& via,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const BI_Zone* boardZone,
                              const BI_Device* zoneDevice,
                              const Zone* deviceZone, const BI_Polygon& polygon,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const BI_Zone* boardZone,
                              const BI_Device* zoneDevice,
                              const Zone* deviceZone, const BI_Device& device,
                              const Polygon& polygon,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const BI_Zone* boardZone,
                              const BI_Device* zoneDevice,
                              const Zone* deviceZone, const BI_Device& device,
                              const Circle& circle,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const DrcMsgExposureInKeepoutZone& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgExposureInKeepoutZone() noexcept {}

private:
  void addZoneApprovalNodes(const BI_Zone* boardZone,
                            const BI_Device* zoneDevice,
                            const Zone* deviceZone) noexcept;
  static QString getDescription() noexcept;
};

/*******************************************************************************
 *  Class DrcMsgMinimumAnnularRingViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgMinimumAnnularRingViolation class
 */
class DrcMsgMinimumAnnularRingViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMinimumAnnularRingViolation)

public:
  // Constructors / Destructor
  DrcMsgMinimumAnnularRingViolation() = delete;
  DrcMsgMinimumAnnularRingViolation(const BI_Via& via,
                                    const UnsignedLength& minAnnularWidth,
                                    const QVector<Path>& locations) noexcept;
  DrcMsgMinimumAnnularRingViolation(const BI_FootprintPad& pad,
                                    const UnsignedLength& minAnnularWidth,
                                    const QVector<Path>& locations) noexcept;
  DrcMsgMinimumAnnularRingViolation(
      const DrcMsgMinimumAnnularRingViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumAnnularRingViolation() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgMinimumDrillDiameterViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgMinimumDrillDiameterViolation class
 */
class DrcMsgMinimumDrillDiameterViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMinimumDrillDiameterViolation)

public:
  // Constructors / Destructor
  DrcMsgMinimumDrillDiameterViolation() = delete;
  DrcMsgMinimumDrillDiameterViolation(const BI_Hole& hole,
                                      const UnsignedLength& minDiameter,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgMinimumDrillDiameterViolation(const BI_Device& device, const Hole& hole,
                                      const UnsignedLength& minDiameter,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgMinimumDrillDiameterViolation(const BI_Via& via,
                                      const UnsignedLength& minDiameter,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgMinimumDrillDiameterViolation(const BI_FootprintPad& pad,
                                      const PadHole& padHole,
                                      const UnsignedLength& minDiameter,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgMinimumDrillDiameterViolation(
      const DrcMsgMinimumDrillDiameterViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumDrillDiameterViolation() noexcept {}

private:
  static QString determineMessage(const PositiveLength& actualDiameter,
                                  const UnsignedLength& minDiameter) noexcept;
  static QString determineDescription(bool isVia, bool isPad) noexcept;
};

/*******************************************************************************
 *  Class DrcMsgMinimumSlotWidthViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgMinimumSlotWidthViolation class
 */
class DrcMsgMinimumSlotWidthViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgMinimumSlotWidthViolation)

public:
  // Constructors / Destructor
  DrcMsgMinimumSlotWidthViolation() = delete;
  DrcMsgMinimumSlotWidthViolation(const BI_Hole& hole,
                                  const UnsignedLength& minWidth,
                                  const QVector<Path>& locations) noexcept;
  DrcMsgMinimumSlotWidthViolation(const BI_Device& device, const Hole& hole,
                                  const UnsignedLength& minWidth,
                                  const QVector<Path>& locations) noexcept;
  DrcMsgMinimumSlotWidthViolation(const BI_FootprintPad& pad,
                                  const PadHole& padHole,
                                  const UnsignedLength& minWidth,
                                  const QVector<Path>& locations) noexcept;
  DrcMsgMinimumSlotWidthViolation(
      const DrcMsgMinimumSlotWidthViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumSlotWidthViolation() noexcept {}

private:
  static QString determineMessage(const PositiveLength& actualWidth,
                                  const UnsignedLength& minWidth) noexcept;
  static QString determineDescription(bool isPad) noexcept;
};

/*******************************************************************************
 *  Class DrcMsgInvalidPadConnection
 ******************************************************************************/

/**
 * @brief The DrcMsgInvalidPadConnection class
 */
class DrcMsgInvalidPadConnection final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgInvalidPadConnection)

public:
  // Constructors / Destructor
  DrcMsgInvalidPadConnection() = delete;
  DrcMsgInvalidPadConnection(const BI_FootprintPad& pad, const Layer& layer,
                             const QVector<Path>& locations) noexcept;
  DrcMsgInvalidPadConnection(const DrcMsgInvalidPadConnection& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgInvalidPadConnection() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgForbiddenSlot
 ******************************************************************************/

/**
 * @brief The DrcMsgForbiddenSlot class
 */
class DrcMsgForbiddenSlot final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgForbiddenSlot)

public:
  // Constructors / Destructor
  DrcMsgForbiddenSlot() = delete;
  DrcMsgForbiddenSlot(const BI_Hole& hole,
                      const QVector<Path>& locations) noexcept;
  DrcMsgForbiddenSlot(const BI_Device& device, const Hole& hole,
                      const QVector<Path>& locations) noexcept;
  DrcMsgForbiddenSlot(const BI_FootprintPad& pad, const PadHole& padHole,
                      const QVector<Path>& locations) noexcept;
  DrcMsgForbiddenSlot(const DrcMsgForbiddenSlot& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgForbiddenSlot() noexcept {}

private:
  static QString determineMessage(const NonEmptyPath& path) noexcept;
  static QString determineDescription(const NonEmptyPath& path) noexcept;
};

/*******************************************************************************
 *  Class DrcMsgForbiddenVia
 ******************************************************************************/

/**
 * @brief The DrcMsgForbiddenVia class
 */
class DrcMsgForbiddenVia final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgForbiddenVia)

public:
  // Constructors / Destructor
  DrcMsgForbiddenVia() = delete;
  DrcMsgForbiddenVia(const BI_Via& via,
                     const QVector<Path>& locations) noexcept;
  DrcMsgForbiddenVia(const DrcMsgForbiddenVia& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgForbiddenVia() noexcept {}

private:
  static QString determineMessage(const BI_Via& via) noexcept;
  static QString determineDescription(const BI_Via& via) noexcept;
};

/*******************************************************************************
 *  Class DrcMsgSilkscreenClearanceViolation
 ******************************************************************************/

/**
 * @brief The DrcMsgSilkscreenClearanceViolation class
 */
class DrcMsgSilkscreenClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgSilkscreenClearanceViolation)

public:
  // Constructors / Destructor
  DrcMsgSilkscreenClearanceViolation() = delete;
  DrcMsgSilkscreenClearanceViolation(const BI_StrokeText& strokeText,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgSilkscreenClearanceViolation(
      const DrcMsgSilkscreenClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgSilkscreenClearanceViolation() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgUselessZone
 ******************************************************************************/

/**
 * @brief The DrcMsgUselessZone class
 */
class DrcMsgUselessZone final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgUselessZone)

public:
  // Constructors / Destructor
  DrcMsgUselessZone() = delete;
  DrcMsgUselessZone(const BI_Zone& zone,
                    const QVector<Path>& locations) noexcept;
  DrcMsgUselessZone(const DrcMsgUselessZone& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgUselessZone() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgUselessVia
 ******************************************************************************/

/**
 * @brief The DrcMsgUselessVia class
 */
class DrcMsgUselessVia final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgUselessVia)

public:
  // Constructors / Destructor
  DrcMsgUselessVia() = delete;
  DrcMsgUselessVia(const BI_Via& via, const QVector<Path>& locations) noexcept;
  DrcMsgUselessVia(const DrcMsgUselessVia& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgUselessVia() noexcept {}
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
