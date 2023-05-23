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

#ifndef LIBREPCB_EDITOR_BOARDCLIPBOARDDATA_H
#define LIBREPCB_EDITOR_BOARDCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/geometry/junction.h>
#include <librepcb/core/geometry/trace.h>
#include <librepcb/core/geometry/via.h>
#include <librepcb/core/project/board/boardholedata.h>
#include <librepcb/core/project/board/boardpolygondata.h>
#include <librepcb/core/project/board/boardstroketextdata.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/serialization/serializableobjectlist.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/utils/signalslot.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class TransactionalDirectory;
class TransactionalFileSystem;

namespace editor {

/*******************************************************************************
 *  Class BoardClipboardData
 ******************************************************************************/

/**
 * @brief The BoardClipboardData class
 */
class BoardClipboardData final {
public:
  // Types
  struct Device {
    static constexpr const char* tagname = "device";

    Uuid componentUuid;
    Uuid libDeviceUuid;
    Uuid libFootprintUuid;
    Point position;
    Angle rotation;
    bool mirrored;
    bool locked;
    AttributeList attributes;
    QList<BoardStrokeTextData> strokeTexts;
    Signal<Device> onEdited;  ///< Dummy event, not used

    Device(const Uuid& componentUuid, const Uuid& libDeviceUuid,
           const Uuid& libFootprintUuid, const Point& position,
           const Angle& rotation, bool mirrored, bool locked,
           const AttributeList& attributes,
           const QList<BoardStrokeTextData>& strokeTexts)
      : componentUuid(componentUuid),
        libDeviceUuid(libDeviceUuid),
        libFootprintUuid(libFootprintUuid),
        position(position),
        rotation(rotation),
        mirrored(mirrored),
        locked(locked),
        attributes(attributes),
        strokeTexts(strokeTexts),
        onEdited(*this) {}

    explicit Device(const SExpression& node)
      : componentUuid(deserialize<Uuid>(node.getChild("@0"))),
        libDeviceUuid(deserialize<Uuid>(node.getChild("lib_device/@0"))),
        libFootprintUuid(deserialize<Uuid>(node.getChild("lib_footprint/@0"))),
        position(node.getChild("position")),
        rotation(deserialize<Angle>(node.getChild("rotation/@0"))),
        mirrored(deserialize<bool>(node.getChild("flip/@0"))),
        locked(deserialize<bool>(node.getChild("lock/@0"))),
        attributes(node),
        strokeTexts(),
        onEdited(*this) {
      foreach (const SExpression* child, node.getChildren("stroke_text")) {
        strokeTexts.append(BoardStrokeTextData(*child));
      }
    }

    void serialize(SExpression& root) const {
      root.appendChild(componentUuid);
      root.ensureLineBreak();
      root.appendChild("lib_device", libDeviceUuid);
      root.ensureLineBreak();
      root.appendChild("lib_footprint", libFootprintUuid);
      root.ensureLineBreak();
      position.serialize(root.appendList("position"));
      root.appendChild("rotation", rotation);
      root.appendChild("flip", mirrored);
      root.appendChild("lock", locked);
      root.ensureLineBreak();
      attributes.serialize(root);
      foreach (const BoardStrokeTextData& strokeText, strokeTexts) {
        root.ensureLineBreak();
        strokeText.serialize(root.appendList("stroke_text"));
      }
      root.ensureLineBreak();
    }

    bool operator!=(const Device& rhs) noexcept {
      return (componentUuid != rhs.componentUuid) ||
          (libDeviceUuid != rhs.libDeviceUuid) ||
          (libFootprintUuid != rhs.libFootprintUuid) ||
          (position != rhs.position) || (rotation != rhs.rotation) ||
          (mirrored != rhs.mirrored) || (locked != rhs.locked) ||
          (attributes != rhs.attributes) || (strokeTexts != rhs.strokeTexts);
    }
  };

  struct NetSegment {
    static constexpr const char* tagname = "netsegment";

    tl::optional<CircuitIdentifier> netName;
    ViaList vias;
    JunctionList junctions;
    TraceList traces;
    Signal<NetSegment> onEdited;  ///< Dummy event, not used

    explicit NetSegment(const tl::optional<CircuitIdentifier>& netName)
      : netName(netName), vias(), junctions(), traces(), onEdited(*this) {}

    explicit NetSegment(const SExpression& node)
      : netName(deserialize<tl::optional<CircuitIdentifier>>(
            node.getChild("net/@0"))),
        vias(node),
        junctions(node),
        traces(node),
        onEdited(*this) {}

    void serialize(SExpression& root) const {
      root.ensureLineBreak();
      root.appendChild("net", netName);
      root.ensureLineBreak();
      vias.serialize(root);
      root.ensureLineBreak();
      junctions.serialize(root);
      root.ensureLineBreak();
      traces.serialize(root);
      root.ensureLineBreak();
    }

    bool operator!=(const NetSegment& rhs) noexcept {
      return (netName != rhs.netName) || (vias != rhs.vias) ||
          (junctions != rhs.junctions) || (traces != rhs.traces);
    }
  };

  struct Plane {
    static constexpr const char* tagname = "plane";

    Uuid uuid;
    const Layer* layer;
    tl::optional<CircuitIdentifier> netSignalName;
    Path outline;
    UnsignedLength minWidth;
    UnsignedLength minClearance;
    bool keepIslands;
    int priority;
    BI_Plane::ConnectStyle connectStyle;
    PositiveLength thermalGap;
    PositiveLength thermalSpokeWidth;
    bool locked;
    Signal<Plane> onEdited;  ///< Dummy event, not used

    Plane(const Uuid& uuid, const Layer& layer,
          const tl::optional<CircuitIdentifier>& netSignalName,
          const Path& outline, const UnsignedLength& minWidth,
          const UnsignedLength& minClearance, bool keepIslands, int priority,
          BI_Plane::ConnectStyle connectStyle, const PositiveLength& thermalGap,
          const PositiveLength& thermalSpokeWidth, bool locked)
      : uuid(uuid),
        layer(&layer),
        netSignalName(netSignalName),
        outline(outline),
        minWidth(minWidth),
        minClearance(minClearance),
        keepIslands(keepIslands),
        priority(priority),
        connectStyle(connectStyle),
        thermalGap(thermalGap),
        thermalSpokeWidth(thermalSpokeWidth),
        locked(locked),
        onEdited(*this) {}

    explicit Plane(const SExpression& node)
      : uuid(deserialize<Uuid>(node.getChild("@0"))),
        layer(deserialize<const Layer*>(node.getChild("layer/@0"))),
        netSignalName(deserialize<tl::optional<CircuitIdentifier>>(
            node.getChild("net/@0"))),
        outline(node),
        minWidth(deserialize<UnsignedLength>(node.getChild("min_width/@0"))),
        minClearance(
            deserialize<UnsignedLength>(node.getChild("min_clearance/@0"))),
        keepIslands(deserialize<bool>(node.getChild("keep_islands/@0"))),
        priority(deserialize<int>(node.getChild("priority/@0"))),
        connectStyle(deserialize<BI_Plane::ConnectStyle>(
            node.getChild("connect_style/@0"))),
        thermalGap(
            deserialize<PositiveLength>(node.getChild("thermal_gap/@0"))),
        thermalSpokeWidth(
            deserialize<PositiveLength>(node.getChild("thermal_spoke/@0"))),
        locked(deserialize<bool>(node.getChild("lock/@0"))),
        onEdited(*this) {}

    void serialize(SExpression& root) const {
      root.appendChild(uuid);
      root.appendChild("layer", *layer);
      root.ensureLineBreak();
      root.appendChild("net", netSignalName);
      root.appendChild("priority", priority);
      root.ensureLineBreak();
      root.appendChild("min_width", minWidth);
      root.appendChild("min_clearance", minClearance);
      root.appendChild("thermal_gap", thermalGap);
      root.appendChild("thermal_spoke", thermalSpokeWidth);
      root.ensureLineBreak();
      root.appendChild("connect_style", connectStyle);
      root.appendChild("keep_islands", keepIslands);
      root.appendChild("lock", locked);
      root.ensureLineBreak();
      outline.serialize(root);
      root.ensureLineBreak();
    }

    bool operator!=(const Plane& rhs) noexcept {
      return (uuid != rhs.uuid) || (layer != rhs.layer) ||
          (netSignalName != rhs.netSignalName) || (outline != rhs.outline) ||
          (minWidth != rhs.minWidth) || (minClearance != rhs.minClearance) ||
          (keepIslands != rhs.keepIslands) || (priority != rhs.priority) ||
          (connectStyle != rhs.connectStyle) ||
          (thermalGap != rhs.thermalGap) ||
          (thermalSpokeWidth != rhs.thermalSpokeWidth) ||
          (locked != rhs.locked);
    }
  };

  // Constructors / Destructor
  BoardClipboardData() = delete;
  BoardClipboardData(const BoardClipboardData& other) = delete;
  BoardClipboardData(const Uuid& boardUuid, const Point& cursorPos) noexcept;
  explicit BoardClipboardData(const QByteArray& mimeData);
  ~BoardClipboardData() noexcept;

  // Getters
  bool isEmpty() const noexcept;
  std::unique_ptr<TransactionalDirectory> getDirectory(
      const QString& path = "") noexcept;
  const Uuid& getBoardUuid() const noexcept { return mBoardUuid; }
  const Point& getCursorPos() const noexcept { return mCursorPos; }
  SerializableObjectList<Device, Device>& getDevices() noexcept {
    return mDevices;
  }
  SerializableObjectList<NetSegment, NetSegment>& getNetSegments() noexcept {
    return mNetSegments;
  }
  SerializableObjectList<Plane, Plane>& getPlanes() noexcept { return mPlanes; }
  QList<BoardPolygonData>& getPolygons() noexcept { return mPolygons; }
  QList<BoardStrokeTextData>& getStrokeTexts() noexcept { return mStrokeTexts; }
  QList<BoardHoleData>& getHoles() noexcept { return mHoles; }
  QMap<std::pair<Uuid, Uuid>, Point>& getPadPositions() noexcept {
    return mPadPositions;
  }

  // General Methods
  std::unique_ptr<QMimeData> toMimeData() const;
  static std::unique_ptr<BoardClipboardData> fromMimeData(
      const QMimeData* mime);

  // Operator Overloadings
  BoardClipboardData& operator=(const BoardClipboardData& rhs) = delete;

private:  // Methods
  static QString getMimeType() noexcept;

private:  // Data
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  Uuid mBoardUuid;
  Point mCursorPos;
  SerializableObjectList<Device, Device> mDevices;
  SerializableObjectList<NetSegment, NetSegment> mNetSegments;
  SerializableObjectList<Plane, Plane> mPlanes;
  QList<BoardPolygonData> mPolygons;
  QList<BoardStrokeTextData> mStrokeTexts;
  QList<BoardHoleData> mHoles;
  QMap<std::pair<Uuid, Uuid>, Point> mPadPositions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
