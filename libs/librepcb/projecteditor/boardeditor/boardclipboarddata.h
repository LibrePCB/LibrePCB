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

#ifndef LIBREPCB_PROJECT_EDITOR_BOARDCLIPBOARDDATA_H
#define LIBREPCB_PROJECT_EDITOR_BOARDCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/geometry/junction.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/geometry/trace.h>
#include <librepcb/common/geometry/via.h>
#include <librepcb/common/signalslot.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>
#include <librepcb/project/boards/items/bi_plane.h>
#include <librepcb/project/circuit/circuit.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TransactionalFileSystem;
class TransactionalDirectory;

namespace project {
namespace editor {

/*******************************************************************************
 *  Class BoardClipboardData
 ******************************************************************************/

/**
 * @brief The BoardClipboardData class
 */
class BoardClipboardData final : public SerializableObject {
public:
  // Types
  struct Device : public SerializableObject {
    static constexpr const char* tagname = "device";

    Uuid componentUuid;
    Uuid libDeviceUuid;
    Uuid libFootprintUuid;
    Point position;
    Angle rotation;
    bool mirrored;
    StrokeTextList strokeTexts;
    Signal<Device> onEdited;  ///< Dummy event, not used

    Device(const Uuid& componentUuid, const Uuid& libDeviceUuid,
           const Uuid& libFootprintUuid, const Point& position,
           const Angle& rotation, bool mirrored,
           const StrokeTextList& strokeTexts)
      : componentUuid(componentUuid),
        libDeviceUuid(libDeviceUuid),
        libFootprintUuid(libFootprintUuid),
        position(position),
        rotation(rotation),
        mirrored(mirrored),
        strokeTexts(strokeTexts),
        onEdited(*this) {}

    explicit Device(const SExpression& node)
      : componentUuid(node.getChildByIndex(0).getValue<Uuid>()),
        libDeviceUuid(node.getValueByPath<Uuid>("lib_device")),
        libFootprintUuid(node.getValueByPath<Uuid>("lib_footprint")),
        position(node.getChildByPath("position")),
        rotation(node.getValueByPath<Angle>("rotation")),
        mirrored(node.getValueByPath<bool>("mirror")),
        strokeTexts(node),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(componentUuid);
      root.appendChild("lib_device", libDeviceUuid, true);
      root.appendChild("lib_footprint", libFootprintUuid, true);
      root.appendChild(position.serializeToDomElement("position"), true);
      root.appendChild("rotation", rotation, false);
      root.appendChild("mirror", mirrored, false);
      strokeTexts.serialize(root);
    }

    bool operator!=(const Device& rhs) noexcept {
      return (componentUuid != rhs.componentUuid) ||
          (libDeviceUuid != rhs.libDeviceUuid) ||
          (libFootprintUuid != rhs.libFootprintUuid) ||
          (position != rhs.position) || (rotation != rhs.rotation) ||
          (mirrored != rhs.mirrored) || (strokeTexts != rhs.strokeTexts);
    }
  };

  struct NetSegment : public SerializableObject {
    static constexpr const char* tagname = "netsegment";

    CircuitIdentifier netName;
    ViaList vias;
    JunctionList junctions;
    TraceList traces;
    Signal<NetSegment> onEdited;  ///< Dummy event, not used

    explicit NetSegment(const CircuitIdentifier& netName)
      : netName(netName), vias(), junctions(), traces(), onEdited(*this) {}

    explicit NetSegment(const SExpression& node)
      : netName(node.getValueByPath<CircuitIdentifier>("net")),
        vias(node),
        junctions(node),
        traces(node),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild("net", netName, true);
      vias.serialize(root);
      junctions.serialize(root);
      traces.serialize(root);
    }

    bool operator!=(const NetSegment& rhs) noexcept {
      return (netName != rhs.netName) || (vias != rhs.vias) ||
          (junctions != rhs.junctions) || (traces != rhs.traces);
    }
  };

  struct Plane : public SerializableObject {
    static constexpr const char* tagname = "plane";

    Uuid uuid;
    QString layer;
    QString netSignalName;
    Path outline;
    UnsignedLength minWidth;
    UnsignedLength minClearance;
    bool keepOrphans;
    int priority;
    BI_Plane::ConnectStyle connectStyle;
    Signal<Plane> onEdited;  ///< Dummy event, not used

    Plane(const Uuid& uuid, const QString& layer, const QString& netSignalName,
          const Path& outline, const UnsignedLength& minWidth,
          const UnsignedLength& minClearance, bool keepOrphans, int priority,
          BI_Plane::ConnectStyle connectStyle)
      : uuid(uuid),
        layer(layer),
        netSignalName(netSignalName),
        outline(outline),
        minWidth(minWidth),
        minClearance(minClearance),
        keepOrphans(keepOrphans),
        priority(priority),
        connectStyle(connectStyle),
        onEdited(*this) {}

    explicit Plane(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        layer(node.getValueByPath<QString>("layer", true)),
        netSignalName(node.getValueByPath<QString>("net", true)),
        outline(node),
        minWidth(node.getValueByPath<UnsignedLength>("min_width")),
        minClearance(node.getValueByPath<UnsignedLength>("min_clearance")),
        keepOrphans(node.getValueByPath<bool>("keep_orphans")),
        priority(node.getValueByPath<int>("priority")),
        connectStyle(
            node.getValueByPath<BI_Plane::ConnectStyle>("connect_style")),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild("layer", layer, false);
      root.appendChild("net", netSignalName, true);
      root.appendChild("priority", priority, false);
      root.appendChild("min_width", minWidth, true);
      root.appendChild("min_clearance", minClearance, false);
      root.appendChild("keep_orphans", keepOrphans, false);
      root.appendChild("connect_style", connectStyle, true);
      outline.serialize(root);
    }

    bool operator!=(const Plane& rhs) noexcept {
      return (uuid != rhs.uuid) || (layer != rhs.layer) ||
          (netSignalName != rhs.netSignalName) || (outline != rhs.outline) ||
          (minWidth != rhs.minWidth) || (minClearance != rhs.minClearance) ||
          (keepOrphans != rhs.keepOrphans) || (priority != rhs.priority) ||
          (connectStyle != rhs.connectStyle);
    }
  };

  // Constructors / Destructor
  BoardClipboardData() = delete;
  BoardClipboardData(const BoardClipboardData& other) = delete;
  BoardClipboardData(const Uuid& boardUuid, const Point& cursorPos) noexcept;
  explicit BoardClipboardData(const QByteArray& mimeData);
  ~BoardClipboardData() noexcept;

  // Getters
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
  PolygonList& getPolygons() noexcept { return mPolygons; }
  StrokeTextList& getStrokeTexts() noexcept { return mStrokeTexts; }
  HoleList& getHoles() noexcept { return mHoles; }
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
  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  static QString getMimeType() noexcept;

private:  // Data
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  Uuid mBoardUuid;
  Point mCursorPos;
  SerializableObjectList<Device, Device> mDevices;
  SerializableObjectList<NetSegment, NetSegment> mNetSegments;
  SerializableObjectList<Plane, Plane> mPlanes;
  PolygonList mPolygons;
  StrokeTextList mStrokeTexts;
  HoleList mHoles;
  QMap<std::pair<Uuid, Uuid>, Point> mPadPositions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
