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

#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/geometry/junction.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/geometry/trace.h>
#include <librepcb/core/geometry/via.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/serialization/serializableobject.h>
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

class TransactionalDirectory;
class TransactionalFileSystem;

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

    Device(const SExpression& node, const Version& fileFormat)
      : componentUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
        libDeviceUuid(
            deserialize<Uuid>(node.getChild("lib_device/@0"), fileFormat)),
        libFootprintUuid(
            deserialize<Uuid>(node.getChild("lib_footprint/@0"), fileFormat)),
        position(node.getChild("position"), fileFormat),
        rotation(deserialize<Angle>(node.getChild("rotation/@0"), fileFormat)),
        mirrored(deserialize<bool>(node.getChild("mirror/@0"), fileFormat)),
        strokeTexts(node, fileFormat),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(componentUuid);
      root.ensureLineBreak();
      root.appendChild("lib_device", libDeviceUuid);
      root.ensureLineBreak();
      root.appendChild("lib_footprint", libFootprintUuid);
      root.ensureLineBreak();
      root.appendChild(position.serializeToDomElement("position"));
      root.appendChild("rotation", rotation);
      root.appendChild("mirror", mirrored);
      root.ensureLineBreak();
      strokeTexts.serialize(root);
      root.ensureLineBreak();
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

    NetSegment(const SExpression& node, const Version& fileFormat)
      : netName(deserialize<CircuitIdentifier>(node.getChild("net/@0"),
                                               fileFormat)),
        vias(node, fileFormat),
        junctions(node, fileFormat),
        traces(node, fileFormat),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
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

  struct Plane : public SerializableObject {
    static constexpr const char* tagname = "plane";

    Uuid uuid;
    GraphicsLayerName layer;
    CircuitIdentifier netSignalName;
    Path outline;
    UnsignedLength minWidth;
    UnsignedLength minClearance;
    bool keepOrphans;
    int priority;
    BI_Plane::ConnectStyle connectStyle;
    Signal<Plane> onEdited;  ///< Dummy event, not used

    Plane(const Uuid& uuid, const GraphicsLayerName& layer,
          const CircuitIdentifier& netSignalName, const Path& outline,
          const UnsignedLength& minWidth, const UnsignedLength& minClearance,
          bool keepOrphans, int priority, BI_Plane::ConnectStyle connectStyle)
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

    Plane(const SExpression& node, const Version& fileFormat)
      : uuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
        layer(deserialize<GraphicsLayerName>(node.getChild("layer/@0"),
                                             fileFormat)),
        netSignalName(deserialize<CircuitIdentifier>(node.getChild("net/@0"),
                                                     fileFormat)),
        outline(node, fileFormat),
        minWidth(deserialize<UnsignedLength>(node.getChild("min_width/@0"),
                                             fileFormat)),
        minClearance(deserialize<UnsignedLength>(
            node.getChild("min_clearance/@0"), fileFormat)),
        keepOrphans(
            deserialize<bool>(node.getChild("keep_orphans/@0"), fileFormat)),
        priority(deserialize<int>(node.getChild("priority/@0"), fileFormat)),
        connectStyle(deserialize<BI_Plane::ConnectStyle>(
            node.getChild("connect_style/@0"), fileFormat)),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild("layer", layer);
      root.ensureLineBreak();
      root.appendChild("net", netSignalName);
      root.appendChild("priority", priority);
      root.ensureLineBreak();
      root.appendChild("min_width", minWidth);
      root.appendChild("min_clearance", minClearance);
      root.appendChild("keep_orphans", keepOrphans);
      root.ensureLineBreak();
      root.appendChild("connect_style", connectStyle);
      root.ensureLineBreak();
      outline.serialize(root);
      root.ensureLineBreak();
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
}  // namespace librepcb

#endif
