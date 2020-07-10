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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATA_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/units/point.h>
#include <librepcb/common/uuid.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/schematics/items/si_symbol.h>

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

    Uuid               uuid;
    Uuid               libComponentUuid;
    Uuid               libVariantUuid;
    tl::optional<Uuid> libDeviceUuid;
    CircuitIdentifier  name;
    QString            value;
    AttributeList      attributes;

    Signal<ComponentInstance> onEdited;  ///< Dummy event, not used

    ComponentInstance(const Uuid& uuid, const Uuid& libComponentUuid,
                      const Uuid&               libVariantUuid,
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

    explicit ComponentInstance(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        libComponentUuid(node.getValueByPath<Uuid>("lib_component")),
        libVariantUuid(node.getValueByPath<Uuid>("lib_variant")),
        libDeviceUuid(node.getValueByPath<tl::optional<Uuid>>("lib_device")),
        name(node.getValueByPath<CircuitIdentifier>("name", true)),
        value(node.getValueByPath<QString>("value")),
        attributes(node),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild("lib_component", libComponentUuid, true);
      root.appendChild("lib_variant", libVariantUuid, true);
      root.appendChild("lib_device", libDeviceUuid, true);
      root.appendChild("name", name, true);
      root.appendChild("value", value, false);
      attributes.serialize(root);
    }
  };

  struct SymbolInstance : public SerializableObject {
    static constexpr const char* tagname = "symbol";

    Uuid  uuid;
    Uuid  componentInstanceUuid;
    Uuid  symbolVariantItemUuid;
    Point position;
    Angle rotation;
    bool  mirrored;

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

    explicit SymbolInstance(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        componentInstanceUuid(node.getValueByPath<Uuid>("component")),
        symbolVariantItemUuid(node.getValueByPath<Uuid>("lib_gate")),
        position(node.getChildByPath("position")),
        rotation(node.getValueByPath<Angle>("rotation")),
        mirrored(node.getValueByPath<bool>("mirror")),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild("component", componentInstanceUuid, true);
      root.appendChild("lib_gate", symbolVariantItemUuid, true);
      root.appendChild(position.serializeToDomElement("position"), true);
      root.appendChild("rotation", rotation, false);
      root.appendChild("mirror", mirrored, false);
    }
  };

  struct NetPoint : public SerializableObject {
    static constexpr const char* tagname = "junction";

    Uuid             uuid;
    Point            position;
    Signal<NetPoint> onEdited;  ///< Dummy event, not used

    NetPoint(const Uuid& uuid, const Point& position)
      : uuid(uuid), position(position), onEdited(*this) {}

    explicit NetPoint(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        position(node.getChildByPath("position")),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild(position.serializeToDomElement("position"), true);
    }
  };

  struct NetLine : public SerializableObject {
    static constexpr const char* tagname = "line";

    Uuid               uuid;
    tl::optional<Uuid> startJunction;
    tl::optional<Uuid> startSymbol;
    tl::optional<Uuid> startPin;
    tl::optional<Uuid> endJunction;
    tl::optional<Uuid> endSymbol;
    tl::optional<Uuid> endPin;
    Signal<NetLine>    onEdited;  ///< Dummy event, not used

    explicit NetLine(const Uuid& uuid)
      : uuid(uuid),
        startJunction(),
        startSymbol(),
        startPin(),
        endJunction(),
        endSymbol(),
        endPin(),
        onEdited(*this) {}

    explicit NetLine(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()), onEdited(*this) {
      if (node.getChildByPath("from").getChildren().count() == 2) {
        startSymbol = node.getValueByPath<Uuid>("from/symbol");
        startPin    = node.getValueByPath<Uuid>("from/pin");
      } else {
        startJunction = node.getValueByPath<Uuid>("from/junction");
      }
      if (node.getChildByPath("to").getChildren().count() == 2) {
        endSymbol = node.getValueByPath<Uuid>("to/symbol");
        endPin    = node.getValueByPath<Uuid>("to/pin");
      } else {
        endJunction = node.getValueByPath<Uuid>("to/junction");
      }
    }

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      SExpression& from = root.appendList("from", true);
      if (startSymbol) {
        from.appendChild("symbol", *startSymbol, false);
      }
      if (startPin) {
        from.appendChild("pin", *startPin, false);
      }
      if (startJunction) {
        from.appendChild("junction", *startJunction, false);
      }
      SExpression& to = root.appendList("to", true);
      if (endSymbol) {
        to.appendChild("symbol", *endSymbol, false);
      }
      if (endPin) {
        to.appendChild("pin", *endPin, false);
      }
      if (endJunction) {
        to.appendChild("junction", *endJunction, false);
      }
    }
  };

  struct NetLabel : public SerializableObject {
    static constexpr const char* tagname = "label";

    Uuid             uuid;
    Point            position;
    Angle            rotation;
    Signal<NetLabel> onEdited;  ///< Dummy event, not used

    NetLabel(const Uuid& uuid, const Point& position, const Angle& rotation)
      : uuid(uuid), position(position), rotation(rotation), onEdited(*this) {}

    explicit NetLabel(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        position(node.getChildByPath("position")),
        rotation(node.getValueByPath<Angle>("rotation")),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild(position.serializeToDomElement("position"), true);
      root.appendChild("rotation", rotation, false);
    }
  };

  struct NetSegment : public SerializableObject {
    static constexpr const char* tagname = "netsegment";

    CircuitIdentifier                          netName;
    SerializableObjectList<NetPoint, NetPoint> points;
    SerializableObjectList<NetLine, NetLine>   lines;
    SerializableObjectList<NetLabel, NetLabel> labels;
    Signal<NetSegment> onEdited;  ///< Dummy event, not used

    explicit NetSegment(const CircuitIdentifier& netName)
      : netName(netName), points(), lines(), labels(), onEdited(*this) {}

    explicit NetSegment(const SExpression& node)
      : netName(node.getValueByPath<CircuitIdentifier>("net")),
        points(node),
        lines(node),
        labels(node),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild("net", netName, true);
      points.serialize(root);
      lines.serialize(root);
      labels.serialize(root);
    }
  };

  // Constructors / Destructor
  SchematicClipboardData()                                    = delete;
  SchematicClipboardData(const SchematicClipboardData& other) = delete;
  SchematicClipboardData(const Uuid&  schematicUuid,
                         const Point& cursorPos) noexcept;
  explicit SchematicClipboardData(const QByteArray& mimeData);
  ~SchematicClipboardData() noexcept;

  // Getters
  std::unique_ptr<TransactionalDirectory> getDirectory(
      const QString& path = "") noexcept;
  const Uuid&  getSchematicUuid() const noexcept { return mSchematicUuid; }
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

  // General Methods
  std::unique_ptr<QMimeData>                     toMimeData() const;
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
  Uuid                                     mSchematicUuid;
  Point                                    mCursorPos;
  SerializableObjectList<ComponentInstance, ComponentInstance>
                                                         mComponentInstances;
  SerializableObjectList<SymbolInstance, SymbolInstance> mSymbolInstances;
  SerializableObjectList<NetSegment, NetSegment>         mNetSegments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATA_H
