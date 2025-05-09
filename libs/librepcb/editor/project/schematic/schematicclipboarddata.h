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
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/componentinstance.h>

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
class SchematicClipboardData final {
public:
  // Types
  struct ComponentInstance {
    static constexpr const char* tagname = "component";

    Uuid uuid;
    Uuid libComponentUuid;
    Uuid libVariantUuid;
    CircuitIdentifier name;
    QString value;
    AttributeList attributes;
    ComponentAssemblyOptionList assemblyOptions;
    bool lockAssembly;

    Signal<ComponentInstance> onEdited;  ///< Dummy event, not used

    ComponentInstance(const Uuid& uuid, const Uuid& libComponentUuid,
                      const Uuid& libVariantUuid, const CircuitIdentifier& name,
                      const QString& value, const AttributeList& attributes,
                      const ComponentAssemblyOptionList& assemblyOptions,
                      bool lockParts)
      : uuid(uuid),
        libComponentUuid(libComponentUuid),
        libVariantUuid(libVariantUuid),
        name(name),
        value(value),
        attributes(attributes),
        assemblyOptions(assemblyOptions),
        lockAssembly(lockParts),
        onEdited(*this) {}

    explicit ComponentInstance(const SExpression& node)
      : uuid(deserialize<Uuid>(node.getChild("@0"))),
        libComponentUuid(deserialize<Uuid>(node.getChild("lib_component/@0"))),
        libVariantUuid(deserialize<Uuid>(node.getChild("lib_variant/@0"))),
        name(deserialize<CircuitIdentifier>(node.getChild("name/@0"))),
        value(node.getChild("value/@0").getValue()),
        attributes(node),
        assemblyOptions(node),
        lockAssembly(deserialize<bool>(node.getChild("lock_assembly/@0"))),
        onEdited(*this) {}

    /// Required for ::librepcb::SerializableObjectList::contains()
    const Uuid& getUuid() const noexcept { return uuid; }

    void serialize(SExpression& root) const {
      root.appendChild(uuid);
      root.ensureLineBreak();
      root.appendChild("lib_component", libComponentUuid);
      root.ensureLineBreak();
      root.appendChild("lib_variant", libVariantUuid);
      root.ensureLineBreak();
      root.appendChild("name", name);
      root.appendChild("value", value);
      root.ensureLineBreak();
      attributes.serialize(root);
      root.ensureLineBreak();
      assemblyOptions.serialize(root);
      root.ensureLineBreak();
      root.appendChild("lock_assembly", lockAssembly);
      root.ensureLineBreak();
    }

    bool operator!=(const ComponentInstance& rhs) noexcept {
      return (uuid != rhs.uuid) || (libComponentUuid != rhs.libComponentUuid) ||
          (libVariantUuid != rhs.libVariantUuid) || (name != rhs.name) ||
          (value != rhs.value) || (attributes != rhs.attributes) ||
          (assemblyOptions != rhs.assemblyOptions) ||
          (lockAssembly != rhs.lockAssembly);
    }
  };

  struct SymbolInstance {
    static constexpr const char* tagname = "symbol";

    Uuid uuid;
    Uuid componentInstanceUuid;
    Uuid symbolVariantItemUuid;
    Point position;
    Angle rotation;
    bool mirrored;
    TextList texts;

    Signal<SymbolInstance> onEdited;  ///< Dummy event, not used

    SymbolInstance(const Uuid& uuid, const Uuid& componentInstanceUuid,
                   const Uuid& symbolVariantItemUuid, const Point& position,
                   const Angle& rotation, bool mirrored, const TextList& texts)
      : uuid(uuid),
        componentInstanceUuid(componentInstanceUuid),
        symbolVariantItemUuid(symbolVariantItemUuid),
        position(position),
        rotation(rotation),
        mirrored(mirrored),
        texts(texts),
        onEdited(*this) {}

    explicit SymbolInstance(const SExpression& node)
      : uuid(deserialize<Uuid>(node.getChild("@0"))),
        componentInstanceUuid(deserialize<Uuid>(node.getChild("component/@0"))),
        symbolVariantItemUuid(deserialize<Uuid>(node.getChild("lib_gate/@0"))),
        position(node.getChild("position")),
        rotation(deserialize<Angle>(node.getChild("rotation/@0"))),
        mirrored(deserialize<bool>(node.getChild("mirror/@0"))),
        texts(node),
        onEdited(*this) {}

    void serialize(SExpression& root) const {
      root.appendChild(uuid);
      root.ensureLineBreak();
      root.appendChild("component", componentInstanceUuid);
      root.ensureLineBreak();
      root.appendChild("lib_gate", symbolVariantItemUuid);
      root.ensureLineBreak();
      position.serialize(root.appendList("position"));
      root.appendChild("rotation", rotation);
      root.appendChild("mirror", mirrored);
      root.ensureLineBreak();
      texts.serialize(root);
      root.ensureLineBreak();
    }

    bool operator!=(const SymbolInstance& rhs) noexcept {
      return (uuid != rhs.uuid) ||
          (componentInstanceUuid != rhs.componentInstanceUuid) ||
          (symbolVariantItemUuid != rhs.symbolVariantItemUuid) ||
          (position != rhs.position) || (rotation != rhs.rotation) ||
          (mirrored != rhs.mirrored) || (texts != rhs.texts);
    }
  };

  struct NetSegment {
    static constexpr const char* tagname = "netsegment";

    CircuitIdentifier netName;
    JunctionList junctions;
    NetLineList lines;
    NetLabelList labels;
    Signal<NetSegment> onEdited;  ///< Dummy event, not used

    explicit NetSegment(const CircuitIdentifier& netName)
      : netName(netName), junctions(), lines(), labels(), onEdited(*this) {}

    explicit NetSegment(const SExpression& node)
      : netName(deserialize<CircuitIdentifier>(node.getChild("net/@0"))),
        junctions(node),
        lines(node),
        labels(node),
        onEdited(*this) {}

    void serialize(SExpression& root) const {
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
  SchematicClipboardData(const Uuid& schematicUuid, const Point& cursorPos,
                         const AssemblyVariantList& assemblyVariants) noexcept;
  explicit SchematicClipboardData(const QByteArray& mimeData);
  ~SchematicClipboardData() noexcept;

  // Getters
  std::unique_ptr<TransactionalDirectory> getDirectory(
      const QString& path = "") noexcept;
  const Uuid& getSchematicUuid() const noexcept { return mSchematicUuid; }
  const Point& getCursorPos() const noexcept { return mCursorPos; }
  const AssemblyVariantList& getAssemblyVariants() noexcept {
    return mAssemblyVariants;
  }
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
  static bool isValid(const QMimeData* mime) noexcept;

  // Operator Overloadings
  SchematicClipboardData& operator=(const SchematicClipboardData& rhs) = delete;

private:  // Methods
  static QString getMimeType() noexcept;

private:  // Data
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  Uuid mSchematicUuid;
  Point mCursorPos;
  AssemblyVariantList mAssemblyVariants;
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
