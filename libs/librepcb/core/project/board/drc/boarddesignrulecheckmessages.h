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
#include "boarddesignrulecheckdata.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class DrcHoleRef
 ******************************************************************************/

struct DrcHoleRef {
  using Data = BoardDesignRuleCheckData;

  bool isPadHole() const noexcept { return mDevice && mPad && mHole; }
  bool isViaHole() const noexcept { return mSegment && mVia; }
  bool isPlated() const noexcept { return isPadHole() || isViaHole(); }
  const QString& getNetName() const noexcept { return mNetName; }
  const Data::Pad* getPad() const noexcept { return mPad; }
  PositiveLength getDiameter() const noexcept {
    if (mHole) {
      return mHole->diameter;
    } else if (mVia) {
      return mVia->drillDiameter;
    } else {
      Q_ASSERT(false);
      qCritical() << "DrcHoleRef: Unknown object type.";
      return PositiveLength(1);
    }
  }
  void serialize(SExpression& node) const;

  static DrcHoleRef boardHole(const Data::Hole& hole) {
    DrcHoleRef obj;
    obj.mHole = &hole;
    return obj;
  }
  static DrcHoleRef deviceHole(const Data::Device& device,
                               const Data::Hole& hole) {
    DrcHoleRef obj;
    obj.mDevice = &device;
    obj.mHole = &hole;
    return obj;
  }
  static DrcHoleRef padHole(const Data::Device& device, const Data::Pad& pad,
                            const Data::Hole& hole) {
    DrcHoleRef obj;
    obj.mDevice = &device;
    obj.mPad = &pad;
    obj.mHole = &hole;
    obj.mNetName = pad.netName;
    return obj;
  }
  static DrcHoleRef via(const Data::Segment& segment, const Data::Via& via) {
    DrcHoleRef obj;
    obj.mSegment = &segment;
    obj.mVia = &via;
    obj.mNetName = segment.netName;
    return obj;
  }

private:
  const Data::Hole* mHole = nullptr;
  const Data::Segment* mSegment = nullptr;
  const Data::Via* mVia = nullptr;
  const Data::Device* mDevice = nullptr;
  const Data::Pad* mPad = nullptr;
  QString mNetName;  // Empty if no net.
};

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
  DrcMsgMissingDevice(const Uuid& uuid, const QString& name) noexcept;
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
  using Data = BoardDesignRuleCheckData;
  struct Anchor {
    QString getName() const;
    void serialize(SExpression& node) const;

    static Anchor pad(const Data::Device& device, const Data::Pad& pad) {
      Anchor obj;
      obj.mDevice = &device;
      obj.mPad = &pad;
      return obj;
    }
    static Anchor junction(const Data::Segment& segment,
                           const Data::Junction& junction) {
      Anchor obj;
      obj.mSegment = &segment;
      obj.mJunction = &junction;
      return obj;
    }
    static Anchor via(const Data::Segment& segment, const Data::Via& via) {
      Anchor obj;
      obj.mSegment = &segment;
      obj.mVia = &via;
      return obj;
    }

  private:
    // Either it's a pad...
    const Data::Device* mDevice = nullptr;
    const Data::Pad* mPad = nullptr;
    // ... or a junction or via
    const Data::Segment* mSegment = nullptr;
    const Data::Junction* mJunction = nullptr;
    const Data::Via* mVia = nullptr;
  };

  // Constructors / Destructor
  DrcMsgMissingConnection() = delete;
  DrcMsgMissingConnection(const Anchor& p1, const Anchor& p2,
                          const QString& netName,
                          const QVector<Path>& locations);
  DrcMsgMissingConnection(const DrcMsgMissingConnection& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMissingConnection() noexcept {}
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
  DrcMsgOpenBoardOutlinePolygon(const Uuid& polygon,
                                const std::optional<Uuid>& device,
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgEmptyNetSegment() = delete;
  explicit DrcMsgEmptyNetSegment(const Data::Segment& ns) noexcept;
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgUnconnectedJunction() = delete;
  DrcMsgUnconnectedJunction(const Data::Junction& junction,
                            const Data::Segment& ns,
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgMinimumTextHeightViolation() = delete;
  DrcMsgMinimumTextHeightViolation(const Data::StrokeText& st,
                                   const Data::Device* device,
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgMinimumWidthViolation() = delete;
  DrcMsgMinimumWidthViolation(const Data::Segment& segment,
                              const Data::Trace& trace,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const Data::Plane& plane,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const Data::Polygon& polygon,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const Data::StrokeText& text,
                              const Data::Device* device,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const Data::Device& device,
                              const Data::Polygon& polygon,
                              const UnsignedLength& minWidth,
                              const QVector<Path>& locations) noexcept;
  DrcMsgMinimumWidthViolation(const Data::Device& device,
                              const Data::Circle& circle,
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
  using Data = BoardDesignRuleCheckData;

  struct Object {
    QString getName() const;
    void serialize(SExpression& node) const;

    static Object pad(const Data::Pad& pad, const Data::Device& device) {
      Object obj;
      obj.mPad = &pad;
      obj.mDevice = &device;
      obj.mNetName = pad.netName;
      return obj;
    }
    static Object trace(const Data::Trace& trace,
                        const Data::Segment& segment) {
      Object obj;
      obj.mTrace = &trace;
      obj.mSegment = &segment;
      obj.mNetName = segment.netName;
      return obj;
    }
    static Object via(const Data::Via& via, const Data::Segment& segment) {
      Object obj;
      obj.mVia = &via;
      obj.mSegment = &segment;
      obj.mNetName = segment.netName;
      return obj;
    }
    static Object plane(const Data::Plane& plane) {
      Object obj;
      obj.mPlane = &plane;
      obj.mNetName = plane.netName;
      return obj;
    }
    static Object polygon(const Data::Polygon& polygon,
                          const Data::Device* device) {
      Object obj;
      obj.mPolygon = &polygon;
      obj.mDevice = device;
      return obj;
    }
    static Object circle(const Data::Circle& circle,
                         const Data::Device* device) {
      Object obj;
      obj.mCircle = &circle;
      obj.mDevice = device;
      return obj;
    }
    static Object strokeText(const Data::StrokeText& txt,
                             const Data::Device* device) {
      Object obj;
      obj.mStrokeText = &txt;
      obj.mDevice = device;
      return obj;
    }

    bool operator==(const Object& rhs) const {
      std::unique_ptr<SExpression> obj1 = SExpression::createList("object");
      serialize(*obj1);
      std::unique_ptr<SExpression> obj2 = SExpression::createList("object");
      rhs.serialize(*obj2);
      return (*obj1) == (*obj2);
    }

  private:
    // Actual object (one of them)
    const Data::Pad* mPad = nullptr;
    const Data::Trace* mTrace = nullptr;
    const Data::Via* mVia = nullptr;
    const Data::Plane* mPlane = nullptr;
    const Data::Polygon* mPolygon = nullptr;
    const Data::Circle* mCircle = nullptr;
    const Data::StrokeText* mStrokeText = nullptr;

    // Optional context (depending on object type)
    const Data::Segment* mSegment = nullptr;
    const Data::Device* mDevice = nullptr;
    QString mNetName;  // Empty if no net.
  };

  // Constructors / Destructor
  DrcMsgCopperCopperClearanceViolation() = delete;
  DrcMsgCopperCopperClearanceViolation(const Object& obj1, const Object& obj2,
                                       const QSet<const Layer*>& layers,
                                       const Length& minClearance,
                                       const QVector<Path>& locations);
  DrcMsgCopperCopperClearanceViolation(
      const DrcMsgCopperCopperClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperCopperClearanceViolation() noexcept {}

private:
  static QString getLayerName(const QSet<const Layer*>& layers);
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgCopperBoardClearanceViolation() = delete;
  DrcMsgCopperBoardClearanceViolation(const Data::Segment& segment,
                                      const Data::Via& via,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const Data::Segment& segment,
                                      const Data::Trace& trace,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const Data::Device& device,
                                      const Data::Pad& pad,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const Data::Plane& plane,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const Data::Polygon& polygon,
                                      const Data::Device* device,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const Data::Device& device,
                                      const Data::Circle& circle,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(const Data::StrokeText& strokeText,
                                      const Data::Device* device,
                                      const UnsignedLength& minClearance,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgCopperBoardClearanceViolation(
      const DrcMsgCopperBoardClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperBoardClearanceViolation() noexcept {}
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgCopperHoleClearanceViolation() = delete;
  DrcMsgCopperHoleClearanceViolation(const Data::Hole& hole,
                                     const Data::Device* device,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgCopperHoleClearanceViolation(
      const DrcMsgCopperHoleClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperHoleClearanceViolation() noexcept {}
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgCopperInKeepoutZone() = delete;
  DrcMsgCopperInKeepoutZone(const Data::Zone& zone,
                            const Data::Device* zoneDevice,
                            const Data::Device& device, const Data::Pad& pad,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const Data::Zone& zone,
                            const Data::Device* zoneDevice,
                            const Data::Segment& ns, const Data::Via& via,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const Data::Zone& zone,
                            const Data::Device* zoneDevice,
                            const Data::Segment& ns, const Data::Trace& trace,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const Data::Zone& zone,
                            const Data::Device* zoneDevice,
                            const Data::Polygon& polygon,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const Data::Zone& zone,
                            const Data::Device* zoneDevice,
                            const Data::Device& device,
                            const Data::Polygon& polygon,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const Data::Zone& zone,
                            const Data::Device* zoneDevice,
                            const Data::Device& device,
                            const Data::Circle& circle,
                            const QVector<Path>& locations) noexcept;
  DrcMsgCopperInKeepoutZone(const DrcMsgCopperInKeepoutZone& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgCopperInKeepoutZone() noexcept {}

private:
  void addZoneApprovalNodes(const Data::Zone& zone,
                            const Data::Device* zoneDevice) noexcept;
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
  DrcMsgDrillDrillClearanceViolation(const DrcHoleRef& hole1,
                                     const DrcHoleRef& hole2,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations);
  DrcMsgDrillDrillClearanceViolation(
      const DrcMsgDrillDrillClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDrillDrillClearanceViolation() noexcept {}
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
  DrcMsgDrillBoardClearanceViolation(const DrcHoleRef& hole,
                                     const UnsignedLength& minClearance,
                                     const QVector<Path>& locations) noexcept;
  DrcMsgDrillBoardClearanceViolation(
      const DrcMsgDrillBoardClearanceViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDrillBoardClearanceViolation() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgDeviceInCourtyard
 ******************************************************************************/

/**
 * @brief The DrcMsgDeviceInCourtyard class
 */
class DrcMsgDeviceInCourtyard final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgDeviceInCourtyard)

public:
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgDeviceInCourtyard() = delete;
  DrcMsgDeviceInCourtyard(const Data::Device& device1,
                          const Data::Device& device2,
                          const QVector<Path>& locations) noexcept;
  DrcMsgDeviceInCourtyard(const DrcMsgDeviceInCourtyard& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDeviceInCourtyard() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgOverlappingDevices
 ******************************************************************************/

/**
 * @brief The DrcMsgOverlappingDevices class
 */
class DrcMsgOverlappingDevices final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgOverlappingDevices)

public:
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgOverlappingDevices() = delete;
  DrcMsgOverlappingDevices(const Data::Device& device1,
                           const Data::Device& device2,
                           const QVector<Path>& locations) noexcept;
  DrcMsgOverlappingDevices(const DrcMsgOverlappingDevices& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgOverlappingDevices() noexcept {}
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgDeviceInKeepoutZone() = delete;
  DrcMsgDeviceInKeepoutZone(const Data::Zone& zone,
                            const Data::Device* zoneDevice,
                            const Data::Device& device,
                            const QVector<Path>& locations) noexcept;
  DrcMsgDeviceInKeepoutZone(const DrcMsgDeviceInKeepoutZone& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDeviceInKeepoutZone() noexcept {}

private:
  void addZoneApprovalNodes(const Data::Zone& zone,
                            const Data::Device* zoneDevice) noexcept;
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgExposureInKeepoutZone() = delete;
  DrcMsgExposureInKeepoutZone(const Data::Zone& zone,
                              const Data::Device* zoneDevice,
                              const Data::Device& device, const Data::Pad& pad,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const Data::Zone& zone,
                              const Data::Device* zoneDevice,
                              const Data::Segment& ns, const Data::Via& via,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const Data::Zone& zone,
                              const Data::Device* zoneDevice,
                              const Data::Polygon& polygon,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const Data::Zone& zone,
                              const Data::Device* zoneDevice,
                              const Data::Device& device,
                              const Data::Polygon& polygon,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const Data::Zone& zone,
                              const Data::Device* zoneDevice,
                              const Data::Device& device,
                              const Data::Circle& circle,
                              const QVector<Path>& locations) noexcept;
  DrcMsgExposureInKeepoutZone(const DrcMsgExposureInKeepoutZone& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgExposureInKeepoutZone() noexcept {}

private:
  void addZoneApprovalNodes(const Data::Zone& zone,
                            const Data::Device* zoneDevice) noexcept;
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgMinimumAnnularRingViolation() = delete;
  DrcMsgMinimumAnnularRingViolation(const Data::Segment& ns,
                                    const Data::Via& via,
                                    const UnsignedLength& minAnnularWidth,
                                    const QVector<Path>& locations) noexcept;
  DrcMsgMinimumAnnularRingViolation(const Data::Device& device,
                                    const Data::Pad& pad,
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
  DrcMsgMinimumDrillDiameterViolation(const DrcHoleRef& hole,
                                      const UnsignedLength& minDiameter,
                                      const QVector<Path>& locations) noexcept;
  DrcMsgMinimumDrillDiameterViolation(
      const DrcMsgMinimumDrillDiameterViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumDrillDiameterViolation() noexcept {}

private:
  static QString determineMessage(const DrcHoleRef& hole,
                                  const UnsignedLength& minDiameter) noexcept;
  static QString determineDescription(const DrcHoleRef& hole) noexcept;
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
  DrcMsgMinimumSlotWidthViolation(const DrcHoleRef& hole,
                                  const UnsignedLength& minWidth,
                                  const QVector<Path>& locations) noexcept;
  DrcMsgMinimumSlotWidthViolation(
      const DrcMsgMinimumSlotWidthViolation& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgMinimumSlotWidthViolation() noexcept {}

private:
  static QString determineMessage(bool plated,
                                  const PositiveLength& actualWidth,
                                  const UnsignedLength& minWidth) noexcept;
  static QString determineDescription(bool plated) noexcept;
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgInvalidPadConnection() = delete;
  DrcMsgInvalidPadConnection(const Data::Device& device, const Data::Pad& pad,
                             const Layer& layer,
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgForbiddenSlot() = delete;
  DrcMsgForbiddenSlot(const Data::Hole& hole, const Data::Device* device,
                      const Data::Pad* pad,
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgForbiddenVia() = delete;
  DrcMsgForbiddenVia(const Data::Segment& ns, const Data::Via& via,
                     const QVector<Path>& locations) noexcept;
  DrcMsgForbiddenVia(const DrcMsgForbiddenVia& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgForbiddenVia() noexcept {}

private:
  static QString determineMessage(const Data::Segment& ns,
                                  const Data::Via& via) noexcept;
  static QString determineDescription(const Data::Via& via) noexcept;
};

/*******************************************************************************
 *  Class DrcMsgInvalidVia
 ******************************************************************************/

/**
 * @brief The DrcMsgInvalidVia class
 */
class DrcMsgInvalidVia final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgInvalidVia)

public:
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgInvalidVia() = delete;
  DrcMsgInvalidVia(const Data::Segment& ns, const Data::Via& via,
                   const QVector<Path>& locations) noexcept;
  DrcMsgInvalidVia(const DrcMsgInvalidVia& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgInvalidVia() noexcept {}
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgSilkscreenClearanceViolation() = delete;
  DrcMsgSilkscreenClearanceViolation(const Data::StrokeText& st,
                                     const Data::Device* device,
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgUselessZone() = delete;
  DrcMsgUselessZone(const Data::Zone& zone,
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
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  DrcMsgUselessVia() = delete;
  DrcMsgUselessVia(const Data::Segment& ns, const Data::Via& via,
                   const QVector<Path>& locations) noexcept;
  DrcMsgUselessVia(const DrcMsgUselessVia& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgUselessVia() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgDisabledLayer
 ******************************************************************************/

/**
 * @brief The DrcMsgDisabledLayer class
 */
class DrcMsgDisabledLayer final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgDisabledLayer)

public:
  // Constructors / Destructor
  DrcMsgDisabledLayer() = delete;
  explicit DrcMsgDisabledLayer(const Layer& layer) noexcept;
  DrcMsgDisabledLayer(const DrcMsgDisabledLayer& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgDisabledLayer() noexcept {}
};

/*******************************************************************************
 *  Class DrcMsgUnusedLayer
 ******************************************************************************/

/**
 * @brief The DrcMsgUnusedLayer class
 */
class DrcMsgUnusedLayer final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(DrcMsgUnusedLayer)

public:
  // Constructors / Destructor
  DrcMsgUnusedLayer() = delete;
  explicit DrcMsgUnusedLayer(const Layer& layer) noexcept;
  DrcMsgUnusedLayer(const DrcMsgUnusedLayer& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~DrcMsgUnusedLayer() noexcept {}
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
