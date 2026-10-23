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
#include "schematictools.h"

#include "cmdnetclassrulesedit.h"
#include "designsession.h"
#include "mcpserver.h"
#include "toolhelpers.h"

#include <librepcb/core/attribute/attrtypestring.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbolpin.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netclass.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/transform.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/editor/cmd/cmdtextedit.h>
#include <librepcb/editor/graphics/graphicslayerlist.h>
#include <librepcb/editor/project/cmd/cmdaddcomponenttocircuit.h>
#include <librepcb/editor/project/cmd/cmdaddsymboltoschematic.h>
#include <librepcb/editor/project/cmd/cmdcombinenetsignals.h>
#include <librepcb/editor/project/cmd/cmdcombineschematicnetsegments.h>
#include <librepcb/editor/project/cmd/cmdcomponentinstanceedit.h>
#include <librepcb/editor/project/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/editor/project/cmd/cmdnetclassadd.h>
#include <librepcb/editor/project/cmd/cmdnetclassedit.h>
#include <librepcb/editor/project/cmd/cmdnetsignaladd.h>
#include <librepcb/editor/project/cmd/cmdnetsignaledit.h>
#include <librepcb/editor/project/cmd/cmdremoveselectedschematicitems.h>
#include <librepcb/editor/project/cmd/cmdschematicadd.h>
#include <librepcb/editor/project/cmd/cmdschematicnetlabeladd.h>
#include <librepcb/editor/project/cmd/cmdschematicnetlabeledit.h>
#include <librepcb/editor/project/cmd/cmdschematicnetpointedit.h>
#include <librepcb/editor/project/cmd/cmdschematicnetsegmentadd.h>
#include <librepcb/editor/project/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/editor/project/cmd/cmdschematictextadd.h>
#include <librepcb/editor/project/cmd/cmdsymbolinstanceeditall.h>
#include <librepcb/editor/project/cmd/cmdsymbolinstancetextadd.h>
#include <librepcb/editor/project/cmd/cmdsymbolinstancetextremove.h>
#include <librepcb/editor/project/projectcrossprobe.h>
#include <librepcb/editor/project/schematic/graphicsitems/sgi_netlabel.h>
#include <librepcb/editor/project/schematic/graphicsitems/sgi_netline.h>
#include <librepcb/editor/project/schematic/graphicsitems/sgi_symbol.h>
#include <librepcb/editor/project/schematic/schematicgraphicsscene.h>
#include <librepcb/editor/undostack.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

using editor::UndoStackTransaction;

/*******************************************************************************
 *  Helpers
 ******************************************************************************/

static QString signalNameOf(const SI_SymbolPin& pin) noexcept {
  return *pin.getComponentSignalInstance().getCompSignal().getName();
}

static SI_SymbolPin& findSymbolPin(Project& project, const QString& ref) {
  const auto [designator, name] = splitPinRef(ref);
  ComponentInstance& cmp = findComponentInstance(project, designator);
  SI_SymbolPin* byPinName = nullptr;
  QStringList available;
  for (const SI_Symbol* symbol : cmp.getSymbols()) {
    for (SI_SymbolPin* pin : symbol->getPins()) {
      if (signalNameOf(*pin) == name) {
        return *pin;
      } else if (*pin->getLibPin().getName() == name) {
        byPinName = pin;
      }
      available.append(signalNameOf(*pin));
    }
  }
  if (byPinName) {
    return *byPinName;
  }
  available.sort();
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Part '%1' has no pin '%2'. Available: %3")
                         .arg(designator, name, available.join(", ")));
}

static QJsonArray pinsToJson(const SI_Symbol& symbol) noexcept {
  QJsonArray pins;
  for (const SI_SymbolPin* pin : symbol.getPins()) {
    const NetSignal* net = pin->getCompSigInstNetSignal();
    pins.append(QJsonObject{
        {"pin", signalNameOf(*pin)},
        {"position", toJson(pin->getPosition())},
        {"net", net ? QJsonValue(*net->getName()) : QJsonValue()},
    });
  }
  return pins;
}

static QString partNumbers(const Device& dev) noexcept {
  QStringList mpns;
  for (const Part& part : dev.getParts()) {
    mpns.append("'" % *part.getMpn() % "'");
  }
  return mpns.join(", ");
}

/**
 * @brief The part chosen by the argument 'part', or the only part of a device
 *
 * @return The part for an assembly option, or nullptr for no part.
 */
static std::shared_ptr<Part> choosePart(const Device& dev,
                                        const QJsonObject& args) {
  const QJsonValue value = args.value("part");
  const QString devName = *dev.getNames().getDefaultValue();
  if (value.isNull() || (value.isUndefined() && dev.getParts().isEmpty())) {
    return nullptr;
  }
  if (value.isObject()) {
    const QJsonObject obj = value.toObject();
    return std::make_shared<Part>(
        SimpleString(cleanSimpleString(requiredString(obj, "mpn"))),
        SimpleString(cleanSimpleString(obj.value("manufacturer").toString())),
        dev.getAttributes());
  }
  if (value.isUndefined() && (dev.getParts().count() > 1)) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("The device '%1' has several parts. Choose one "
                               "with the argument 'part': %2 (or null for "
                               "none).")
                           .arg(devName, partNumbers(dev)));
  }
  const QString mpn = value.isUndefined() ? *dev.getParts().first()->getMpn()
                                          : value.toString().trimmed();
  for (const Part& part : dev.getParts()) {
    if (part.getMpn()->compare(mpn, Qt::CaseInsensitive) == 0) {
      return std::make_shared<Part>(part.getMpn(), part.getManufacturer(),
                                    part.getAttributes() | dev.getAttributes());
    }
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("The device '%1' has no part '%2'. Available: %3")
          .arg(devName, mpn,
               dev.getParts().isEmpty() ? "none" : partNumbers(dev)));
}

static QJsonObject partToJson(const ComponentInstance& cmp) noexcept {
  QJsonArray symbols;
  for (const SI_Symbol* symbol : cmp.getSymbols()) {
    symbols.append(QJsonObject{
        {"schematic", *symbol->getSchematic().getName()},
        {"gate", *symbol->getCompSymbVarItem().getSuffix()},
        {"position", toJson(symbol->getPosition())},
        {"rotation", symbol->getRotation().toDeg()},
        {"mirrored", symbol->getMirrored()},
        {"pins", pinsToJson(*symbol)},
    });
  }
  std::shared_ptr<const Part> part;
  if (const auto option = cmp.getAssemblyOptions().value(0)) {
    part = option->getParts().value(0);
  }
  QJsonObject attributes;
  for (const Attribute& attr : cmp.getAttributes()) {
    attributes.insert(*attr.getKey(), attr.getValueTr(true));
  }
  return QJsonObject{
      {"designator", *cmp.getName()},
      {"value", partValue(cmp)},
      {"mpn", part ? QJsonValue(*part->getMpn()) : QJsonValue()},
      {"manufacturer",
       part ? QJsonValue(*part->getManufacturer()) : QJsonValue()},
      {"attributes", attributes},
      {"symbols", symbols},
  };
}

/**
 * @brief Set attribute values, add new text attributes or remove (null) ones
 */
static void setAttributes(ComponentInstance& cmp, const QJsonObject& values,
                          editor::CmdComponentInstanceEdit& cmd) {
  AttributeList attributes = cmp.getAttributes();
  for (auto it = values.begin(); it != values.end(); ++it) {
    const std::optional<AttributeKey> key =
        parseAttributeKey(cleanAttributeKey(it.key().toUpper()));
    if (!key) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Invalid attribute name '%1'.").arg(it.key()));
    }
    std::shared_ptr<Attribute> attr = attributes.find(**key);
    if (it.value().isNull()) {
      if (attr) attributes.remove(attr.get());
      continue;
    }
    if (!attr) {
      attr = std::make_shared<Attribute>(*key, AttrTypeString::instance(),
                                         QString(), nullptr);
      attributes.append(attr);
    }
    QString value = it.value().toString().trimmed();
    const AttributeUnit* unit = attr->getType().tryExtractUnitFromValue(value);
    if (!unit) {
      unit =
          attr->getUnit() ? attr->getUnit() : attr->getType().getDefaultUnit();
    }
    if (!attr->getType().isValueValid(value)) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Invalid value '%1' for attribute '%2'.")
                             .arg(it.value().toString(), it.key()));
    }
    *attr = Attribute(attr->getKey(), attr->getType(), value, unit);
  }
  cmd.setAttributes(attributes);
}

/**
 * @brief Assembly options with the part given by the argument 'part'
 *
 * The part is searched in the devices of all assembly options.
 */
static ComponentAssemblyOptionList assemblyOptionsWithPart(
    DesignSession& session, const ComponentInstance& cmp,
    const QJsonObject& args) {
  ComponentAssemblyOptionList options = cmp.getAssemblyOptions();
  if (options.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       "The part has no device yet, choose one with "
                       "board_place_device first.");
  }
  if (args.value("part").isNull()) {
    for (ComponentAssemblyOption& option : options) {
      option.getParts().clear();
    }
    return options;
  }
  const WorkspaceLibraryDb& db = session.getWorkspace().getLibraryDb();
  QStringList errors;
  for (ComponentAssemblyOption& option : options) {
    std::unique_ptr<Device> dev =
        openElement<Device>(db.getLatest<Device>(option.getDevice()));
    try {
      std::shared_ptr<Part> part = choosePart(*dev, args);  // can throw
      option.getParts().clear();
      option.getParts().append(part);
      return options;
    } catch (const Exception& e) {
      errors.append(e.getMsg());
    }
  }
  throw RuntimeError(__FILE__, __LINE__, errors.join(" "));
}

/**
 * @brief Outward direction of a pin, i.e. from the body towards the wire end
 */
static Point pinDirection(const SI_SymbolPin& pin) {
  const SymbolPin& libPin = pin.getLibPin();
  const Point inner = libPin.getPosition() +
      Point(*libPin.getLength(), 0).rotated(libPin.getRotation());
  const Point delta = pin.getPosition() - Transform(pin.getSymbol()).map(inner);
  const Length dx = delta.getX(), dy = delta.getY();
  if (dx.abs() >= dy.abs()) {
    return Point((dx >= 0) ? 1 : -1, 0);
  } else {
    return Point(0, (dy >= 0) ? 1 : -1);
  }
}

static NetClass& defaultNetClass(Circuit& circuit,
                                 UndoStackTransaction& transaction) {
  if (NetClass* netclass = circuit.getNetClassByName(ElementName("default"))) {
    return *netclass;
  }
  auto* cmd = new editor::CmdNetClassAdd(circuit, ElementName("default"));
  transaction.append(cmd);  // can throw
  return *cmd->getNetClass();
}

/**
 * @brief End point and net label placement of a stub wire at a pin
 */
static std::pair<Point, NetLabel> stubGeometry(const SI_SymbolPin& pin) {
  const Point dir = pinDirection(pin);
  const Point end = pin.getPosition() +
      Point(dir.getX() * Length(2540000), dir.getY() * Length(2540000));
  const bool vertical = (dir.getX() == 0);
  const bool backwards = (dir.getX() < 0) || (dir.getY() < 0);
  return std::make_pair(
      end,
      NetLabel(Uuid::createRandom(), end,
               vertical ? Angle::deg90() : Angle::deg0(), backwards));
}

/**
 * @brief A wire from a pin to a net point, alone in its net segment
 */
struct Stub {
  SI_SymbolPin* pin;
  SI_NetPoint* point;
  QList<SI_NetLabel*> labels;
};

static std::optional<Stub> findStub(SI_SymbolPin& pin) noexcept {
  SI_NetSegment* segment = pin.getNetSegmentOfLines();
  if ((!segment) || (pin.getNetLines().count() != 1) ||
      (segment->getNetLines().count() != 1)) {
    return std::nullopt;
  }
  SI_NetLine* line = *pin.getNetLines().begin();
  SI_NetPoint* point = dynamic_cast<SI_NetPoint*>(line->getOtherPoint(pin));
  if (!point) {
    return std::nullopt;
  }
  return Stub{&pin, point, segment->getNetLabels().values()};
}

/**
 * @brief Connect a pin with a short wire and a net label to a net
 */
static void addStub(SI_SymbolPin& pin, NetSignal& net,
                    UndoStackTransaction& transaction) {
  if (pin.getCompSigInstNetSignal() != &net) {
    transaction.append(new editor::CmdCompSigInstSetNetSignal(
        pin.getComponentSignalInstance(), &net));  // can throw
  }
  const auto [end, label] = stubGeometry(pin);
  auto* cmdSegment = new editor::CmdSchematicNetSegmentAdd(
      pin.getSymbol().getSchematic(), net);
  transaction.append(cmdSegment);  // can throw
  SI_NetSegment& segment = *cmdSegment->getNetSegment();
  auto* cmdElements = new editor::CmdSchematicNetSegmentAddElements(segment);
  SI_NetPoint* point = cmdElements->addNetPoint(end);
  cmdElements->addNetLine(pin, *point);
  transaction.append(cmdElements);  // can throw
  transaction.append(new editor::CmdSchematicNetLabelAdd(
      *new SI_NetLabel(segment, label)));  // can throw
}

/**
 * @brief Remove schematic items like the editor removes selected items
 *
 * The editor's command works on the selection of a graphics scene, so a
 * temporary scene of the schematic gets the items selected.
 */
static void removeSchematicItems(Schematic& schematic,
                                 const QSet<SI_Symbol*>& symbols,
                                 const QSet<SI_NetLine*>& lines,
                                 const QSet<SI_NetLabel*>& labels,
                                 UndoStackTransaction& transaction) {
  const std::unique_ptr<editor::GraphicsLayerList> layers =
      editor::GraphicsLayerList::schematicLayers(nullptr);
  const bool ignorePlacementLocks = true;
  editor::SchematicGraphicsScene scene(
      schematic, *layers,
      std::make_shared<editor::SchematicGraphicsScene::Context>(
          editor::SchematicGraphicsScene::Context{
              nullptr,  // tab
              std::make_shared<editor::ProjectCrossProbe>(),  // cross probe
              editor::GraphicsLayer::State::Enabled,  // self-probe mode
              ignorePlacementLocks,  // ignore placement locks
          }));
  auto select = [](const auto& items, const auto& selection) {
    for (auto* obj : selection) {
      auto item = items.value(obj);
      if (!item) throw LogicError(__FILE__, __LINE__);
      item->setSelected(true);
    }
  };
  select(scene.getSymbols(), symbols);  // can throw
  select(scene.getNetLines(), lines);  // can throw
  select(scene.getNetLabels(), labels);  // can throw
  transaction.append(
      new editor::CmdRemoveSelectedSchematicItems(scene));  // can throw
}

/**
 * @brief Remove all wires attached to a pin, including now unused labels
 */
static void removeWires(SI_SymbolPin& pin, UndoStackTransaction& transaction) {
  const QSet<SI_NetLine*> lines = pin.getNetLines();
  if (lines.isEmpty()) {
    return;
  }
  SI_NetSegment* segment = pin.getNetSegmentOfLines();
  Q_ASSERT(segment);
  QSet<SI_NetLabel*> labels;
  if (Toolbox::toSet(segment->getNetLines().values()) == lines) {
    labels = Toolbox::toSet(segment->getNetLabels().values());
  }
  removeSchematicItems(pin.getSymbol().getSchematic(), {}, lines, labels,
                       transaction);  // can throw
}

static NetSignal* determineNet(const QList<SI_SymbolPin*>& pins,
                               QString& name) {
  NetSignal* net = nullptr;
  for (const SI_SymbolPin* pin : pins) {
    const QString forced =
        pin->getComponentSignalInstance().getForcedNetSignalName();
    if ((!forced.isEmpty()) && (forced != name)) {
      if (!name.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__,
                           QString("Pin '%1' requires the net name '%2'.")
                               .arg(signalNameOf(*pin), forced));
      }
      name = forced;
    }
    if (NetSignal* existing = pin->getCompSigInstNetSignal()) {
      if (net && (existing != net)) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("The pins are already connected to different nets ('%1' "
                    "and '%2'). Disconnect them first.")
                .arg(*net->getName(), *existing->getName()));
      }
      net = existing;
    }
  }
  return net;
}

/**
 * @brief Give a net the requested or forced name. A net with an automatic
 *        name merges into an existing net of that name.
 */
static NetSignal& nameNet(Circuit& circuit, NetSignal& net, const QString& name,
                          UndoStackTransaction& transaction) {
  if (name.isEmpty() || (*net.getName() == name)) {
    return net;
  }
  if (NetSignal* existing = circuit.getNetSignalByName(name)) {
    if (!net.hasAutoName()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Net '%1' cannot be merged into the existing net '%2', "
                  "only nets with an automatic name like 'N1' are merged.")
              .arg(*net.getName(), name));
    }
    transaction.append(new editor::CmdCombineNetSignals(
        circuit, net, *existing));  // can throw
    return *existing;
  }
  auto* cmd = new editor::CmdNetSignalEdit(circuit, net);
  cmd->setName(CircuitIdentifier(name), false);  // can throw
  transaction.append(cmd);  // can throw
  return net;
}

/**
 * @brief The symbol of a gate of a part, or the only symbol
 */
static SI_Symbol& findSymbol(ComponentInstance& cmp, const QJsonValue& gate) {
  QStringList gates;
  SI_Symbol* found = nullptr;
  for (SI_Symbol* symbol : cmp.getSymbols()) {
    const QString suffix = *symbol->getCompSymbVarItem().getSuffix();
    gates.append("'" % suffix % "'");
    if (gate.isUndefined() ? (cmp.getSymbols().count() == 1)
                           : (suffix == gate.toString())) {
      found = symbol;
    }
  }
  if (found) {
    return *found;
  }
  gates.sort();
  if (gates.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__, "The part has no symbol.");
  } else if (gate.isUndefined()) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("The part has several gates, choose one with "
                               "'gate': %1")
                           .arg(gates.join(", ")));
  }
  throw RuntimeError(__FILE__, __LINE__,
                     QString("The part has no gate '%1'. Available: %2")
                         .arg(gate.toString(), gates.join(", ")));
}

static const QJsonObject sNameOrIndex{
    {"type", QJsonArray{"string", "integer"}},
    {"description", "Name or index, default: the first one"},
};

static const QJsonObject sGate{
    {"type", "string"},
    {"description",
     "Gate (symbol suffix, e.g. 'A') of a part with several symbols"},
};

static QJsonObject partProp(const QString& description) noexcept {
  return QJsonObject{
      {"type", QJsonArray{"string", "null", "object"}},
      {"properties",
       QJsonObject{{"mpn", QJsonObject{{"type", "string"}}},
                   {"manufacturer", QJsonObject{{"type", "string"}}}}},
      {"required", QJsonArray{"mpn"}},
      {"description", description},
  };
}

/*******************************************************************************
 *  Functions
 ******************************************************************************/

void addSchematicTools(McpServer& server, DesignSession& session) noexcept {
  server.addTool(McpTool{
      "project_create",
      "Create a new project with one schematic page ('Main') and one board "
      "('default', 100x80mm outline) and open it.",
      objectSchema({{"path",
                     stringProp("Absolute path of the new *.lpp file, its "
                                "directory must not exist yet or "
                                "be empty")},
                    {"name", stringProp("Project name")}},
                   {"path", "name"}),
      false,
      [&session](const QJsonObject& args) {
        session.createProject(absoluteFilePath(requiredString(args, "path")),
                              requiredString(args, "name"));
        return McpToolResult::text("Project created and opened.");
      },
  });

  server.addTool(McpTool{
      "undo",
      "Undo the last change.",
      QJsonObject(),
      false,
      [&session](const QJsonObject&) {
        editor::UndoStack& stack = session.getUndoStack();
        if (!stack.canUndo()) {
          throw RuntimeError(__FILE__, __LINE__, "Nothing to undo.");
        }
        const QString text = stack.getUndoCmdText();
        stack.undo();  // can throw
        return McpToolResult::text("Undone: " % text);
      },
  });

  server.addTool(McpTool{
      "redo",
      "Redo the last undone change.",
      QJsonObject(),
      false,
      [&session](const QJsonObject&) {
        editor::UndoStack& stack = session.getUndoStack();
        if (!stack.canRedo()) {
          throw RuntimeError(__FILE__, __LINE__, "Nothing to redo.");
        }
        const QString text = stack.getRedoCmdText();
        stack.redo();  // can throw
        return McpToolResult::text("Redone: " % text);
      },
  });

  server.addTool(McpTool{
      "schematic_add_part",
      "Add a part (a component from the workspace libraries) to a schematic "
      "page. Choose a device to define the footprint and part number now, "
      "or later with board_place_device. Returns the designator and the "
      "pin positions.",
      objectSchema(
          {{"component",
            stringProp("Name or UUID of the component, e.g. "
                       "'Capacitor' or 'Diode'")},
           {"device", stringProp("Optional name or UUID of the device")},
           {"part",
            partProp("Part to order: an MPN of the device (required "
                     "if the device has several parts), any part as "
                     "{\"mpn\": ..., \"manufacturer\": ...}, or "
                     "null for none.")},
           {"symbolVariant",
            stringProp("Optional symbol variant, as listed by library_get "
                       "for the component")},
           {"designator", stringProp("Optional designator, e.g. 'R1'")},
           {"value", stringProp("Optional value text")},
           {"attributes",
            QJsonObject{
                {"type", "object"},
                {"additionalProperties", QJsonObject{{"type", "string"}}},
                {"description",
                 "Attribute values with unit suffix, e.g. "
                 "{\"RESISTANCE\": \"100k\"} or {\"CAPACITANCE\": \"4.7u\"}"},
            }},
           {"schematic", sNameOrIndex},
           {"position", pointProp("Position, on the 2.54mm grid")},
           {"rotation", numberProp("Rotation in degrees")},
           {"mirror", boolProp("Mirror the symbol")}},
          {"component", "position"}),
      false,
      [&session](const QJsonObject& args) {
        Workspace& ws = session.getWorkspace();
        Project& project = session.getProject();
        Circuit& circuit = project.getCircuit();
        const WorkspaceLibraryDb& db = ws.getLibraryDb();
        Schematic& schematic = session.getSchematic(args.value("schematic"));
        const auto cmpFound = findLibraryElement<Component>(
            db, requiredString(args, "component"));
        std::unique_ptr<Component> libCmp =
            openElement<Component>(cmpFound.second);

        // Symbol variant.
        const ComponentSymbolVariant* variant = nullptr;
        const QString variantName = args.value("symbolVariant").toString();
        for (const ComponentSymbolVariant& var : libCmp->getSymbolVariants()) {
          if (variantName.isEmpty() ||
              (var.getNames().getDefaultValue()->compare(
                   variantName, Qt::CaseInsensitive) == 0)) {
            variant = &var;
            if (variantName.isEmpty()) {
              const int index = std::max(
                  libCmp->getSymbolVariantIndexByNorm(project.getNormOrder()),
                  0);
              variant = libCmp->getSymbolVariants().value(index).get();
            }
            break;
          }
        }
        if (!variant) {
          QStringList names;
          for (const ComponentSymbolVariant& var :
               libCmp->getSymbolVariants()) {
            names.append(*var.getNames().getDefaultValue());
          }
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Symbol variant not found. Available: %1")
                                 .arg(names.join(", ")));
        }

        // Assembly options.
        ComponentAssemblyOptionList options;
        const QString deviceName = args.value("device").toString();
        if (!deviceName.isEmpty()) {
          const auto devFound = findLibraryElement<Device>(db, deviceName);
          std::unique_ptr<Device> libDev = openElement<Device>(devFound.second);
          if (libDev->getComponentUuid() != cmpFound.first) {
            throw RuntimeError(
                __FILE__, __LINE__,
                QString("The device '%1' is not a device of the component.")
                    .arg(deviceName));
          }
          PartList parts;
          if (std::shared_ptr<Part> part = choosePart(*libDev, args)) {
            parts.append(part);
          }
          std::unique_ptr<Package> libPkg = openElement<Package>(
              db.getLatest<Package>(libDev->getPackageUuid()));
          QSet<Uuid> assemblyVariants;
          if (libPkg->getAssemblyType(true) != Package::AssemblyType::None) {
            assemblyVariants = circuit.getAssemblyVariants().getUuidSet();
          }
          options.append(std::make_shared<ComponentAssemblyOption>(
              devFound.first, libDev->getAttributes(), assemblyVariants,
              parts));
        } else if (args.contains("part")) {
          throw RuntimeError(__FILE__, __LINE__,
                             "Choose a device to choose a part.");
        }

        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: schematic_add_part");
        auto* cmdAdd = new editor::CmdAddComponentToCircuit(
            ws, project, cmpFound.first, variant->getUuid(), options);
        transaction.append(cmdAdd);  // can throw
        ComponentInstance& cmp = *cmdAdd->getComponentInstance();

        auto edit =
            std::make_unique<editor::CmdComponentInstanceEdit>(circuit, cmp);
        const QString designator = args.value("designator").toString();
        if ((!designator.isEmpty()) && (designator != *cmp.getName())) {
          if (circuit.getComponentInstanceByName(designator)) {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("The designator '%1' is already used.")
                                   .arg(designator));
          }
          edit->setName(CircuitIdentifier(designator));  // can throw
        }
        if (args.contains("value")) {
          edit->setValue(args.value("value").toString());
        }
        setAttributes(cmp, args.value("attributes").toObject(), *edit);
        transaction.append(edit.release());  // can throw

        const Point position = pointMm(args.value("position"), "position");
        const Angle rotation = angleDeg(args.value("rotation"), "rotation");
        for (const ComponentSymbolVariantItem& item :
             variant->getSymbolItems()) {
          auto* cmdSym = new editor::CmdAddSymbolToSchematic(
              ws, schematic, cmp, item.getUuid(),
              position + item.getSymbolPosition().rotated(rotation), rotation);
          transaction.append(cmdSym);  // can throw
          if (args.value("mirror").toBool(false)) {
            auto* cmdMirror = new editor::CmdSymbolInstanceEditAll(
                *cmdSym->getSymbolInstance());
            cmdMirror->setMirrored(true, true);
            transaction.append(cmdMirror);  // can throw
          }
        }
        transaction.commit();  // can throw
        return McpToolResult::json(partToJson(cmp));
      },
  });

  server.addTool(McpTool{
      "schematic_connect",
      "Connect pins to one net. Each pin gets a short wire with a net label, "
      "so pins with the same label are connected without drawing wires "
      "across the page. Pins which already have a wire are kept as they are. "
      "Supply symbols (e.g. 'Supply GND') force their net name. Pins in a "
      "net with an automatic name like 'N1' join the net of the given or "
      "forced name.",
      objectSchema({{"pins",
                     arrayProp(stringProp("Pin, e.g. 'U1.VCC' or 'R1.1'"),
                               "Pins to connect")},
                    {"net",
                     stringProp("Optional net name, e.g. 'VCC'. Without a "
                                "name an automatic name like 'N3' is used.")}},
                   {"pins"}),
      false,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        Circuit& circuit = project.getCircuit();
        QList<SI_SymbolPin*> pins;
        const QJsonArray pinRefs = args.value("pins").toArray();
        for (const QJsonValue& ref : pinRefs) {
          SI_SymbolPin* pin = &findSymbolPin(project, ref.toString());
          if (!pins.contains(pin)) {
            pins.append(pin);
          }
        }
        if (pins.isEmpty()) {
          throw RuntimeError(__FILE__, __LINE__, "No pins given.");
        }
        QString name = args.value("net").toString().trimmed();
        NetSignal* net = determineNet(pins, name);

        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: schematic_connect");
        if (net) {
          net = &nameNet(circuit, *net, name, transaction);  // can throw
        } else {
          net = name.isEmpty() ? nullptr : circuit.getNetSignalByName(name);
          if (!net) {
            std::optional<CircuitIdentifier> id;
            if (!name.isEmpty()) {
              id = CircuitIdentifier(name);  // can throw
            }
            auto* cmd = new editor::CmdNetSignalAdd(
                circuit, defaultNetClass(circuit, transaction), id);
            transaction.append(cmd);  // can throw
            net = cmd->getNetSignal();
          }
        }
        for (SI_SymbolPin* pin : std::as_const(pins)) {
          if (pin->getNetLines().isEmpty()) {
            addStub(*pin, *net, transaction);  // can throw
          }
        }
        transaction.commit();  // can throw
        return McpToolResult::json(QJsonObject{{"net", *net->getName()}});
      },
  });

  server.addTool(McpTool{
      "schematic_draw_wire",
      "Draw a wire between two pins on the same schematic page, optionally "
      "through corners, and connect them to one net: a pin without net joins "
      "the net of the other pin, otherwise a new net is created. Pins of "
      "different nets cannot be wired (disconnect one first). Supply "
      "symbols force their net name, a net with an automatic name like 'N1' "
      "joins the net of that name. Returns the net.",
      objectSchema({{"from", stringProp("Pin, e.g. 'U1.VCC'")},
                    {"to", stringProp("Pin, e.g. 'C1.1'")},
                    {"points",
                     arrayProp(pointProp("Corner"),
                               "Optional corners, on the 2.54mm grid")}},
                   {"from", "to"}),
      false,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        Circuit& circuit = project.getCircuit();
        SI_SymbolPin* a = &findSymbolPin(project, requiredString(args, "from"));
        SI_SymbolPin* b = &findSymbolPin(project, requiredString(args, "to"));
        QList<Point> points;
        const QJsonArray pointValues = args.value("points").toArray();
        for (const QJsonValue& value : pointValues) {
          points.append(pointMm(value, "points"));
        }
        if (a == b) {
          throw RuntimeError(__FILE__, __LINE__, "The pins are the same.");
        }
        Schematic& schematic = a->getSymbol().getSchematic();
        if (&b->getSymbol().getSchematic() != &schematic) {
          throw RuntimeError(__FILE__, __LINE__,
                             "The pins are on different schematic pages, "
                             "connect them with schematic_connect.");
        }
        QString name;
        NetSignal* net = determineNet({a, b}, name);  // can throw

        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: schematic_draw_wire");
        if (net) {
          net = &nameNet(circuit, *net, name, transaction);  // can throw
        } else if (!name.isEmpty()) {
          net = circuit.getNetSignalByName(name);
        }
        if (!net) {
          std::optional<CircuitIdentifier> id;
          if (!name.isEmpty()) {
            id = CircuitIdentifier(name);  // can throw
          }
          auto* cmd = new editor::CmdNetSignalAdd(
              circuit, defaultNetClass(circuit, transaction), id);
          transaction.append(cmd);  // can throw
          net = cmd->getNetSignal();
        }
        for (SI_SymbolPin* pin : {a, b}) {
          if (pin->getCompSigInstNetSignal() != net) {
            transaction.append(new editor::CmdCompSigInstSetNetSignal(
                pin->getComponentSignalInstance(), net));  // can throw
          }
        }
        if ((!a->getNetSegmentOfLines()) && b->getNetSegmentOfLines()) {
          std::swap(a, b);
          std::reverse(points.begin(), points.end());
        }
        SI_NetSegment* segA = a->getNetSegmentOfLines();
        SI_NetSegment* segB = b->getNetSegmentOfLines();
        SI_NetSegment* segment = segA;
        if (!segment) {
          auto* cmd = new editor::CmdSchematicNetSegmentAdd(schematic, *net);
          transaction.append(cmd);  // can throw
          segment = cmd->getNetSegment();
        }
        const bool combine = segA && segB && (segA != segB);
        auto* cmdAdd = new editor::CmdSchematicNetSegmentAddElements(*segment);
        SI_NetLineAnchor* previous = a;
        for (const Point& pos : points) {
          SI_NetPoint* point = cmdAdd->addNetPoint(pos);
          cmdAdd->addNetLine(*previous, *point);
          previous = point;
        }
        SI_NetPoint* temporary =
            combine ? cmdAdd->addNetPoint(b->getPosition()) : nullptr;
        cmdAdd->addNetLine(*previous,
                           temporary
                               ? static_cast<SI_NetLineAnchor&>(*temporary)
                               : static_cast<SI_NetLineAnchor&>(*b));
        transaction.append(cmdAdd);  // can throw
        if (combine) {
          transaction.append(new editor::CmdCombineSchematicNetSegments(
              *segment, *temporary, *segB, *b));  // can throw
        }
        transaction.commit();  // can throw
        return McpToolResult::json(QJsonObject{{"net", *net->getName()}});
      },
  });

  server.addTool(McpTool{
      "schematic_add_text",
      "Add a text (e.g. a note or a block title) to a schematic page.",
      objectSchema({{"text", stringProp("The text")},
                    {"position", pointProp("Bottom left corner of the text")},
                    {"rotation", numberProp("Rotation in degrees")},
                    {"height", numberProp("Text height in mm, default 2.5")},
                    {"schematic", sNameOrIndex}},
                   {"text", "position"}),
      false,
      [&session](const QJsonObject& args) {
        Schematic& schematic = session.getSchematic(args.value("schematic"));
        auto* text = new SI_Text(
            schematic,
            Text(Uuid::createRandom(), Layer::schematicComments(),
                 requiredString(args, "text"),
                 pointMm(args.value("position"), "position"),
                 angleDeg(args.value("rotation"), "rotation"),
                 PositiveLength(args.contains("height")
                                    ? lengthMm(args.value("height"), "height")
                                    : Length(2500000)),
                 Alignment(HAlign::left(), VAlign::bottom()),
                 false));  // can throw
        session.execCmd(new editor::CmdSchematicTextAdd(*text));  // can throw
        return McpToolResult::text("Text added.");
      },
  });

  server.addTool(McpTool{
      "schematic_add_page",
      "Add a schematic page. Nets with the same name connect across pages.",
      objectSchema({{"name", stringProp("Page name, e.g. 'Power Supply'")}},
                   {"name"}),
      false,
      [&session](const QJsonObject& args) {
        const ElementName name(cleanElementName(requiredString(args, "name")));
        session.execCmd(new editor::CmdSchematicAdd(
            session.getProject(),
            FilePath::cleanFileName(
                *name, FilePath::ReplaceSpaces | FilePath::ToLowerCase),
            name));  // can throw
        return McpToolResult::text("Page added.");
      },
  });

  server.addTool(McpTool{
      "schematic_set_part_text",
      "Move, rotate, resize, hide or show the designator ('name') or value "
      "text of a part's symbol in the schematic.",
      objectSchema({{"designator", stringProp("e.g. 'R1'")},
                    {"gate", sGate},
                    {"text", enumProp({"name", "value"}, "Which text")},
                    {"position", pointProp("New position")},
                    {"rotation", numberProp("New rotation in degrees")},
                    {"height", numberProp("New text height in mm")},
                    {"visible", boolProp("Show or remove the text")}},
                   {"designator", "text"}),
      false,
      [&session](const QJsonObject& args) {
        ComponentInstance& cmp = findComponentInstance(
            session.getProject(), requiredString(args, "designator"));
        SI_Symbol& symbol = findSymbol(cmp, args.value("gate"));
        const QString placeholder =
            (args.value("text") == "value") ? "{{VALUE}}" : "{{NAME}}";
        const bool visible = args.value("visible").toBool(true);
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: schematic_set_part_text");
        QList<SI_Text*> texts;
        for (SI_Text* text : symbol.getTexts()) {
          if (text->getTextObj().getText() == placeholder) texts.append(text);
        }
        if (visible && texts.isEmpty()) {
          for (const Text& text : symbol.getDefaultTexts()) {
            if (text.getText() == placeholder) {
              auto* t = new SI_Text(symbol.getSchematic(),
                                    Text(Uuid::createRandom(), text));
              transaction.append(
                  new editor::CmdSymbolInstanceTextAdd(symbol, *t));
              texts.append(t);
            }
          }
        }
        for (SI_Text* text : texts) {
          if (!visible) {
            transaction.append(
                new editor::CmdSymbolInstanceTextRemove(symbol, *text));
            continue;
          }
          auto* cmd = new editor::CmdTextEdit(text->getTextObj());
          if (args.contains("position")) {
            cmd->setPosition(pointMm(args.value("position"), "position"),
                             false);
          }
          if (args.contains("rotation")) {
            cmd->setRotation(angleDeg(args.value("rotation"), "rotation"),
                             false);
          }
          if (args.contains("height")) {
            cmd->setHeight(
                PositiveLength(lengthMm(args.value("height"), "height")),
                false);  // can throw
          }
          transaction.append(cmd);  // can throw
        }
        transaction.commit();  // can throw
        return McpToolResult::text(visible ? "Text changed." : "Text removed.");
      },
  });

  server.addTool(McpTool{
      "schematic_disconnect",
      "Remove all wires from pins and disconnect them from their net.",
      objectSchema(
          {{"pins",
            arrayProp(stringProp("Pin, e.g. 'U1.VCC'"), "Pins to disconnect")}},
          {"pins"}),
      false,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: schematic_disconnect");
        const QJsonArray pinRefs = args.value("pins").toArray();
        for (const QJsonValue& ref : pinRefs) {
          SI_SymbolPin& pin = findSymbolPin(project, ref.toString());
          removeWires(pin, transaction);  // can throw
          if (pin.getCompSigInstNetSignal()) {
            transaction.append(new editor::CmdCompSigInstSetNetSignal(
                pin.getComponentSignalInstance(), nullptr));  // can throw
          }
        }
        transaction.commit();  // can throw
        return McpToolResult::text("Disconnected.");
      },
  });

  server.addTool(McpTool{
      "net_rename",
      "Rename a net. A net with an automatic name like 'N1' merges into an "
      "existing net of the new name.",
      objectSchema({{"net", stringProp("Current net name")},
                    {"name", stringProp("New net name, e.g. 'SPI_SCK'")}},
                   {"net", "name"}),
      false,
      [&session](const QJsonObject& args) {
        Circuit& circuit = session.getProject().getCircuit();
        NetSignal* net =
            circuit.getNetSignalByName(requiredString(args, "net"));
        if (!net) {
          throw RuntimeError(__FILE__, __LINE__, "Net not found.");
        }
        const QString name = requiredString(args, "name");
        if (circuit.getNetSignalByName(name) && (!net->hasAutoName())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("A net named '%1' exists already.").arg(name));
        }
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: net_rename");
        nameNet(circuit, *net, name, transaction);  // can throw
        transaction.commit();  // can throw
        return McpToolResult::text("Net renamed.");
      },
  });

  auto nullableNumber = [](const QString& description) {
    return QJsonObject{{"type", QJsonArray{"number", "null"}},
                       {"description", description}};
  };
  server.addTool(McpTool{
      "net_class_set",
      "Create or change a net class and move nets into it, e.g. a class for "
      "power nets with wider traces. Nets start in the class 'default'. "
      "Trace width and via drill are defaults for new traces and vias "
      "(board_route, board_add_via); clearance, width and via drill minimums "
      "are checked by the DRC in addition to the board's DRC settings.",
      objectSchema(
          {{"name", stringProp("Net class name, e.g. 'Power'")},
           {"newName", stringProp("Optional new name")},
           {"defaultTraceWidth",
            nullableNumber("Default trace width in mm, null for the board "
                           "default")},
           {"defaultViaDrill",
            nullableNumber("Default via drill in mm, null for the board "
                           "default")},
           {"minClearance",
            numberProp("Minimum copper clearance to other nets in mm")},
           {"minWidth", numberProp("Minimum copper width in mm")},
           {"minViaDrill", numberProp("Minimum via drill in mm")},
           {"nets", arrayProp(stringProp("Net name"), "Nets to move here")}},
          {"name"}),
      false,
      [&session](const QJsonObject& args) {
        Circuit& circuit = session.getProject().getCircuit();
        const ElementName name(cleanElementName(requiredString(args, "name")));
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: net_class_set");
        NetClass* netclass = circuit.getNetClassByName(name);
        if (!netclass) {
          auto* cmd = new editor::CmdNetClassAdd(circuit, name);
          transaction.append(cmd);  // can throw
          netclass = cmd->getNetClass();
        }
        auto optionalLength =
            [&args](const QString& key) -> std::optional<PositiveLength> {
          if (args.value(key).isNull()) return std::nullopt;
          return PositiveLength(lengthMm(args.value(key), key));  // can throw
        };
        auto edit = std::make_unique<editor::CmdNetClassEdit>(*netclass);
        if (args.contains("newName")) {
          edit->setName(
              ElementName(cleanElementName(requiredString(args, "newName"))));
        }
        if (args.contains("defaultTraceWidth")) {
          edit->setDefaultTraceWidth(optionalLength("defaultTraceWidth"));
        }
        if (args.contains("defaultViaDrill")) {
          edit->setDefaultViaDrill(optionalLength("defaultViaDrill"));
        }
        transaction.append(edit.release());  // can throw
        auto rules = std::make_unique<CmdNetClassRulesEdit>(*netclass);
        if (args.contains("minClearance")) {
          rules->setMinCopperCopperClearance(UnsignedLength(
              lengthMm(args.value("minClearance"), "minClearance")));
        }
        if (args.contains("minWidth")) {
          rules->setMinCopperWidth(
              UnsignedLength(lengthMm(args.value("minWidth"), "minWidth")));
        }
        if (args.contains("minViaDrill")) {
          rules->setMinViaDrillDiameter(UnsignedLength(
              lengthMm(args.value("minViaDrill"), "minViaDrill")));
        }
        transaction.append(rules.release());  // can throw
        // The net class of a net cannot change, so the net is replaced by a
        // new one in the class.
        const QJsonArray netValues = args.value("nets").toArray();
        for (const QJsonValue& value : netValues) {
          NetSignal* net = circuit.getNetSignalByName(value.toString());
          if (!net) {
            throw RuntimeError(
                __FILE__, __LINE__,
                QString("Net '%1' not found.").arg(value.toString()));
          }
          if (&net->getNetClass() == netclass) continue;
          const CircuitIdentifier netName = net->getName();
          const bool autoName = net->hasAutoName();
          auto* cmdAdd = new editor::CmdNetSignalAdd(circuit, *netclass);
          transaction.append(cmdAdd);  // can throw
          NetSignal* newNet = cmdAdd->getNetSignal();
          transaction.append(new editor::CmdCombineNetSignals(
              circuit, *net, *newNet));  // can throw
          auto* cmdName = new editor::CmdNetSignalEdit(circuit, *newNet);
          cmdName->setName(netName, autoName);
          transaction.append(cmdName);  // can throw
        }
        transaction.commit();  // can throw
        return McpToolResult::text("Net class changed.");
      },
  });

  server.addTool(McpTool{
      "part_get",
      "Get the value, symbols and pins (position and net) of a part.",
      objectSchema({{"designator", stringProp("e.g. 'U1'")}}, {"designator"}),
      true,
      [&session](const QJsonObject& args) {
        return McpToolResult::json(partToJson(findComponentInstance(
            session.getProject(), requiredString(args, "designator"))));
      },
  });

  server.addTool(McpTool{
      "part_set_properties",
      "Change designator, value, attributes or the ordered part number of a "
      "part.",
      objectSchema(
          {{"designator", stringProp("Current designator, e.g. 'R1'")},
           {"newDesignator", stringProp("Optional new designator")},
           {"part",
            partProp("Optional part to order: an MPN of the chosen device, "
                     "any part as {\"mpn\": ..., \"manufacturer\": ...}, or "
                     "null for none")},
           {"value", stringProp("Optional new value text")},
           {"attributes",
            QJsonObject{
                {"type", "object"},
                {"additionalProperties",
                 QJsonObject{{"type", QJsonArray{"string", "null"}}}},
                {"description",
                 "Attribute values, e.g. {\"RESISTANCE\": \"2.2k\"}. New "
                 "names add text attributes (e.g. {\"TOLERANCE\": "
                 "\"1%\"}), null removes one."},
            }}},
          {"designator"}),
      false,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        Circuit& circuit = project.getCircuit();
        ComponentInstance& cmp =
            findComponentInstance(project, requiredString(args, "designator"));
        auto cmd =
            std::make_unique<editor::CmdComponentInstanceEdit>(circuit, cmp);
        const QString designator = args.value("newDesignator").toString();
        if (!designator.isEmpty()) {
          if (circuit.getComponentInstanceByName(designator)) {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("The designator '%1' is already used.")
                                   .arg(designator));
          }
          cmd->setName(CircuitIdentifier(designator));  // can throw
        }
        if (args.contains("value")) {
          cmd->setValue(args.value("value").toString());
        }
        setAttributes(cmp, args.value("attributes").toObject(), *cmd);
        if (args.contains("part")) {
          cmd->setAssemblyOptions(
              assemblyOptionsWithPart(session, cmp, args));  // can throw
        }
        session.execCmd(cmd.release());  // can throw
        return McpToolResult::json(partToJson(cmp));
      },
  });

  server.addTool(McpTool{
      "schematic_move_part",
      "Move, rotate or mirror the symbol of a part (one gate of a part with "
      "several symbols). Short wires with net labels created by "
      "schematic_connect move along, other wires stay attached to the pins. "
      "Board traces are not affected.",
      objectSchema({{"designator", stringProp("e.g. 'U1'")},
                    {"gate", sGate},
                    {"position", pointProp("New position")},
                    {"rotation", numberProp("New rotation in degrees")},
                    {"mirror", boolProp("Mirrored or not")}},
                   {"designator", "position"}),
      false,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        ComponentInstance& cmp =
            findComponentInstance(project, requiredString(args, "designator"));
        SI_Symbol* symbol = &findSymbol(cmp, args.value("gate"));
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: schematic_move_part");
        // Note: Removing and re-adding the wires would disconnect the pins
        // temporarily, which removes the traces from the boards.
        QList<Stub> stubs;
        for (SI_SymbolPin* pin : symbol->getPins()) {
          if (const std::optional<Stub> stub = findStub(*pin)) {
            stubs.append(*stub);
          }
        }
        auto* cmd = new editor::CmdSymbolInstanceEditAll(*symbol);
        cmd->setPosition(pointMm(args.value("position"), "position"), true);
        if (args.contains("rotation")) {
          cmd->setRotation(angleDeg(args.value("rotation"), "rotation"), true);
        }
        if (args.contains("mirror")) {
          cmd->setMirrored(args.value("mirror").toBool(), true);
        }
        transaction.append(cmd);  // can throw
        for (const Stub& stub : stubs) {
          const auto [end, label] = stubGeometry(*stub.pin);
          auto* cmdPoint = new editor::CmdSchematicNetPointEdit(*stub.point);
          cmdPoint->setPosition(end, false);
          transaction.append(cmdPoint);  // can throw
          for (SI_NetLabel* netLabel : stub.labels) {
            auto* cmdLabel = new editor::CmdSchematicNetLabelEdit(*netLabel);
            if (netLabel->getMirrored() != label.getMirrored()) {
              cmdLabel->mirror(Qt::Horizontal, netLabel->getPosition(), false);
            }
            cmdLabel->setPosition(end, false);
            cmdLabel->setRotation(label.getRotation(), false);
            transaction.append(cmdLabel);  // can throw
          }
        }
        transaction.commit();  // can throw
        return McpToolResult::json(partToJson(cmp));
      },
  });

  server.addTool(McpTool{
      "part_remove",
      "Remove a part from the schematics (and boards) including its wires.",
      objectSchema({{"designator", stringProp("e.g. 'R1'")}}, {"designator"}),
      false,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        ComponentInstance& cmp =
            findComponentInstance(project, requiredString(args, "designator"));
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: part_remove");
        const QList<SI_Symbol*> symbols = cmp.getSymbols().values();
        for (SI_Symbol* symbol : symbols) {
          for (SI_SymbolPin* pin : symbol->getPins()) {
            removeWires(*pin, transaction);  // can throw
          }
        }
        for (SI_Symbol* symbol : symbols) {
          removeSchematicItems(symbol->getSchematic(), {symbol}, {}, {},
                               transaction);  // can throw
        }
        transaction.commit();  // can throw
        return McpToolResult::text("Part removed.");
      },
  });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
