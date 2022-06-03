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

#ifndef LIBREPCB_EDITOR_SCHEMATICCLIPBOARDDATA_H
#define LIBREPCB_EDITOR_SCHEMATICCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <librepcb/core/geometry/junction.h>
#include <librepcb/core/geometry/netlabel.h>
#include <librepcb/core/geometry/netline.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/text.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/serialization/serializableobject.h>

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
 *  Class SchematicClipboardData
 ******************************************************************************/

/**
 * @brief The SchematicClipboardData class
 */
class SchematicClipboardData final : public SerializableObject {
public:
  // Types
  struct ComponentInstance : public SerializableObject {
    static constexpr const char* tagname = "component";

    Uuid uuid;
    Uuid libComponentUuid;
    Uuid libVariantUuid;
    tl::optional<Uuid> libDeviceUuid;
    CircuitIdentifier name;
    QString value;
    AttributeList attributes;

    Signal<ComponentInstance> onEdited;  ///< Dummy event, not used

    ComponentInstance(const Uuid& uuid, const Uuid& libComponentUuid,
                      const Uuid& libVariantUuid,
                      const tl::optional<Uuid>& libDeviceUuid,
                      const CircuitIdentifier& name, const QString& value,
                      const AttributeList& attributes)
      : uuid(uuid),
        libComponentUuid(libComponentUuid),
        libVariantUuid(libVariantUuid),
        libDeviceUuid(libDeviceUuid),
        name(name),
        value(value),
        attributes(attributes),
        onEdited(*this) {}

    ComponentInstance(const SExpression& node, const Version& fileFormat)
      : uuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
        libComponentUuid(
            deserialize<Uuid>(node.getChild("lib_component/@0"), fileFormat)),
        libVariantUuid(
            deserialize<Uuid>(node.getChild("lib_variant/@0"), fileFormat)),
        libDeviceUuid(deserialize<tl::optional<Uuid>>(
            node.getChild("lib_device/@0"), fileFormat)),
        name(deserialize<CircuitIdentifier>(node.getChild("name/@0"),
                                            fileFormat)),
        value(node.getChild("value/@0").getValue()),
        attributes(node, fileFormat),
        onEdited(*this) {}

    /// Required for ::librepcb::SerializableObjectList::contains()
    const Uuid& getUuid() const noexcept { return uuid; }

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.ensureLineBreak();
      root.appendChild("lib_component", libComponentUuid);
      root.ensureLineBreak();
      root.appendChild("lib_variant", libVariantUuid);
      root.ensureLineBreak();
      root.appendChild("lib_device", libDeviceUuid);
      root.ensureLineBreak();
      root.appendChild("name", name);
      root.appendChild("value", value);
      root.ensureLineBreak();
      attributes.serialize(root);
      root.ensureLineBreak();
    }

    bool operator!=(const ComponentInstance& rhs) noexcept {
      return (uuid != rhs.uuid) || (libComponentUuid != rhs.libComponentUuid) ||
          (libVariantUuid != rhs.libVariantUuid) ||
          (libDeviceUuid != rhs.libDeviceUuid) || (name != rhs.name) ||
          (value != rhs.value) || (attributes != rhs.attributes);
    }
  };

  struct SymbolInstance : public SerializableObject {
    static constexpr const char* tagname = "symbol";

    Uuid uuid;
    Uuid componentInstanceUuid;
    Uuid symbolVariantItemUuid;
    Point position;
    Angle rotation;
    bool mirrored;

    Signal<SymbolInstance> onEdited;  ///< Dummy event, not used

    SymbolInstance(const Uuid& uuid, const Uuid& componentInstanceUuid,
                   const Uuid& symbolVariantItemUuid, const Point& position,
                   const Angle& rotation, bool mirrored)
      : uuid(uuid),
        componentInstanceUuid(componentInstanceUuid),
        symbolVariantItemUuid(symbolVariantItemUuid),
        position(position),
        rotation(rotation),
        mirrored(mirrored),
        onEdited(*this) {}

    SymbolInstance(const SExpression& node, const Version& fileFormat)
      : uuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
        componentInstanceUuid(
            deserialize<Uuid>(node.getChild("component/@0"), fileFormat)),
        symbolVariantItemUuid(
            deserialize<Uuid>(node.getChild("lib_gate/@0"), fileFormat)),
        position(node.getChild("position"), fileFormat),
        rotation(deserialize<Angle>(node.getChild("rotation/@0"), fileFormat)),
        mirrored(deserialize<bool>(node.getChild("mirror/@0"), fileFormat)),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.ensureLineBreak();
      root.appendChild("component", componentInstanceUuid);
      root.ensureLineBreak();
      root.appendChild("lib_gate", symbolVariantItemUuid);
      root.ensureLineBreak();
      root.appendChild(position.serializeToDomElement("position"));
      root.appendChild("rotation", rotation);
      root.appendChild("mirror", mirrored);
      root.ensureLineBreak();
    }

    bool operator!=(const SymbolInstance& rhs) noexcept {
      return (uuid != rhs.uuid) ||
          (componentInstanceUuid != rhs.componentInstanceUuid) ||
          (symbolVariantItemUuid != rhs.symbolVariantItemUuid) ||
          (position != rhs.position) || (rotation != rhs.rotation) ||
          (mirrored != rhs.mirrored);
    }
  };

  struct NetSegment : public SerializableObject {
    static constexpr const char* tagname = "netsegment";

    CircuitIdentifier netName;
    JunctionList junctions;
    NetLineList lines;
    NetLabelList labels;
    Signal<NetSegment> onEdited;  ///< Dummy event, not used

    explicit NetSegment(const CircuitIdentifier& netName)
      : netName(netName), junctions(), lines(), labels(), onEdited(*this) {}

    NetSegment(const SExpression& node, const Version& fileFormat)
      : netName(deserialize<CircuitIdentifier>(node.getChild("net/@0"),
                                               fileFormat)),
        junctions(node, fileFormat),
        lines(node, fileFormat),
        labels(node, fileFormat),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.ensureLineBreak();
      root.appendChild("net", netName);
      root.ensureLineBreak();
      junctions.serialize(root);
      root.ensureLineBreak();
      lines.serialize(root);
      root.ensureLineBreak();
      labels.serialize(root);
      root.ensureLineBreak();
    }

    bool operator!=(const NetSegment& rhs) noexcept {
      return (netName != rhs.netName) || (junctions != rhs.junctions) ||
          (lines != rhs.lines) || (labels != rhs.labels);
    }
  };

  // Constructors / Destructor
  SchematicClipboardData() = delete;
  SchematicClipboardData(const SchematicClipboardData& other) = delete;
  SchematicClipboardData(const Uuid& schematicUuid,
                         const Point& cursorPos) noexcept;
  explicit SchematicClipboardData(const QByteArray& mimeData);
  ~SchematicClipboardData() noexcept;

  // Getters
  std::unique_ptr<TransactionalDirectory> getDirectory(
      const QString& path = "") noexcept;
  const Uuid& getSchematicUuid() const noexcept { return mSchematicUuid; }
  const Point& getCursorPos() const noexcept { return mCursorPos; }
  SerializableObjectList<ComponentInstance, ComponentInstance>&
      getComponentInstances() noexcept {
    return mComponentInstances;
  }
  SerializableObjectList<SymbolInstance, SymbolInstance>&
      getSymbolInstances() noexcept {
    return mSymbolInstances;
  }
  SerializableObjectList<NetSegment, NetSegment>& getNetSegments() noexcept {
    return mNetSegments;
  }
  PolygonList& getPolygons() noexcept { return mPolygons; }
  TextList& getTexts() noexcept { return mTexts; }

  // General Methods
  std::unique_ptr<QMimeData> toMimeData() const;
  static std::unique_ptr<SchematicClipboardData> fromMimeData(
      const QMimeData* mime);

  // Operator Overloadings
  SchematicClipboardData& operator=(const SchematicClipboardData& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  static QString getMimeType() noexcept;

private:  // Data
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  Uuid mSchematicUuid;
  Point mCursorPos;
  SerializableObjectList<ComponentInstance, ComponentInstance>
      mComponentInstances;
  SerializableObjectList<SymbolInstance, SymbolInstance> mSymbolInstances;
  SerializableObjectList<NetSegment, NetSegment> mNetSegments;
  PolygonList mPolygons;
  TextList mTexts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
