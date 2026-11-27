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
#include "symbolinstancerpc.h"

#include "../../guiapplication.h"
#include "../../library/libraryelementcache.h"
#include "../../project/cmd/cmdaddcomponenttocircuit.h"
#include "../../project/cmd/cmdaddsymboltoschematic.h"
#include "../../project/cmd/cmdcomponentinstanceedit.h"
#include "../../project/cmd/cmdsymbolinstanceeditall.h"
#include "../../project/projecteditor.h"
#include "../../project/schematic/schematiceditor.h"
#include "../../undostack.h"
#include "../rpcobjectregistry.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpchelpers.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/dev/part.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

static SchematicEditor& getObj(RpcObjectRegistry& registry,
                               RpcRequest& request) {
  return registry.getAs<SchematicEditor>(request.getArg("obj"));
}

static QVariantMap summary(SI_Symbol& obj, RpcObjectRegistry& registry) {
  return {
      {"obj", registry.getHandle(obj)},
      {"uuid", toVariant(obj.getUuid())},
      {"component", toVariant(obj.getComponentInstance().getUuid())},
      {"name", obj.getName()},
      {"position", toVariant(obj.getPosition())},
      {"rotation", toVariant(obj.getRotation())},
      {"mirror", obj.getMirrored()},
  };
}

static QVariantMap details(SI_Symbol& obj, RpcObjectRegistry& registry) {
  ComponentInstance& cmp = obj.getComponentInstance();
  QVariantMap result = summary(obj, registry);
  result.insert("value", cmp.getValue());
  result.insert("attributes",
                toVariant(cmp.getAttributes(),
                          [](const Attribute& obj) { return toVariant(obj); }));
  return result;
}

/// Parse a `Point` argument, i.e. an array `[<int>, <int>]` in nanometers.
static Point getPointArg(const RpcRequest& request, const QString& name) {
  const QVariant value = request.getArg(name);
  const QVariantList list = value.toList();
  if ((value.userType() != QMetaType::QVariantList) || (list.count() != 2)) {
    throw RpcInvalidArgs(
        QString("Invalid value for argument '%1' (expected array with "
                "two integers).")
            .arg(name));
  }
  return Point(Length(list.at(0).toLongLong()),
               Length(list.at(1).toLongLong()));
}

/// Parse an `Angle` argument, i.e. an `int` in microdegrees.
static Angle getAngleArg(const RpcRequest& request, const QString& name) {
  const QVariant value = request.getArg(name);
  if (value.userType() != QMetaType::LongLong) {
    throw RpcInvalidArgs(
        QString("Invalid value for argument '%1' (expected an integer).")
            .arg(name));
  }
  return Angle(static_cast<qint32>(value.toLongLong()));
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void SymbolInstanceRpc::list(GuiApplication& app, RpcObjectRegistry& registry,
                             RpcRequest& request) {
  Q_UNUSED(app);
  SchematicEditor& editor = getObj(registry, request);

  QVariantList symbols;
  for (SI_Symbol* obj : editor.getSchematic().getSymbols()) {
    symbols.append(summary(*obj, registry));
  }
  request.send({{"symbols", symbols}});
}

void SymbolInstanceRpc::add(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  SchematicEditor& editor = getObj(registry, request);
  Schematic& schematic = editor.getSchematic();
  Project& project = schematic.getProject();
  Workspace& workspace = editor.getProjectEditor().getWorkspace();
  UndoStack& undoStack = editor.getProjectEditor().getUndoStack();
  const auto& cache = app.getLibraryElementCache();

  const Uuid componentUuid = getArg<Uuid>(request, "component");
  const Uuid symbVarUuid = getArg<Uuid>(request, "representation");
  const Uuid gateUuid = getArg<Uuid>(request, "gate");
  const auto deviceUuid = getOptionalArg<Uuid>(request, "device");
  const auto part = getOptionalArg<QVariantMap>(request, "part");
  const Point position = getPointArg(request, "position");
  const Angle rotation = getAngleArg(request, "rotation");
  const bool mirror = getArg<bool>(request, "mirror");
  const AttributeList attributes = getArg<AttributeList>(request, "attributes");

  // Resolve the device and build the assembly options from it (if a device
  // was provided, otherwise a schematic-only component is added).
  std::optional<ComponentAssemblyOptionList> assemblyOptions;
  if (deviceUuid) {
    auto libDev = cache.get<Device>(*deviceUuid, true);  // can throw
    PartList parts;
    if (part) {
      parts.append(std::make_shared<Part>(
          SimpleString(part->value("mpn").toString()),
          SimpleString(part->value("manufacturer").toString()),
          AttributeList()));
    }
    QSet<Uuid> assemblyVariants;
    if (auto libPkg = cache.get<Package>(libDev->getPackageUuid(), false)) {
      if (libPkg->getAssemblyType(true) != Package::AssemblyType::None) {
        assemblyVariants =
            project.getCircuit().getAssemblyVariants().getUuidSet();
      }
    }
    ComponentAssemblyOptionList options;
    options.append(std::make_shared<ComponentAssemblyOption>(
        libDev->getUuid(), libDev->getAttributes(), assemblyVariants, parts));
    assemblyOptions = options;
  }

  // Add the component to the circuit and the symbol to the schematic.
  SI_Symbol* symbol = nullptr;
  undoStack.beginCmdGroup(tr("Add symbol to schematic"));  // can throw
  try {
    auto cmdAddCmp = new CmdAddComponentToCircuit(
        workspace, project, componentUuid, symbVarUuid, assemblyOptions);
    undoStack.appendToCmdGroup(cmdAddCmp);  // executes immediately
    auto cmdAddSym = new CmdAddSymbolToSchematic(
        workspace, schematic, *cmdAddCmp->getComponentInstance(), gateUuid,
        position);
    undoStack.appendToCmdGroup(cmdAddSym);  // executes immediately
    symbol = cmdAddSym->getSymbolInstance();

    auto cmdEditSym = new CmdSymbolInstanceEditAll(*symbol);
    cmdEditSym->setRotation(rotation, true);
    cmdEditSym->setMirrored(mirror, true);
    undoStack.appendToCmdGroup(cmdEditSym);

    auto cmdEditCmp = new CmdComponentInstanceEdit(
        project.getCircuit(), *cmdAddCmp->getComponentInstance());
    cmdEditCmp->setAttributes(attributes);
    undoStack.appendToCmdGroup(cmdEditCmp);

    undoStack.commitCmdGroup();
  } catch (...) {
    try {
      undoStack.abortCmdGroup();
    } catch (...) {
    }
    throw;
  }

  request.send(details(*symbol, registry));
}

void SymbolInstanceRpc::add_gate(GuiApplication& app,
                                 RpcObjectRegistry& registry,
                                 RpcRequest& request) {
  SchematicEditor& editor = getObj(registry, request);
  Schematic& schematic = editor.getSchematic();
  UndoStack& undoStack = editor.getProjectEditor().getUndoStack();

  const Uuid cmpInstanceUuid = getArg<Uuid>(request, "component");
  const Uuid gateUuid = getArg<Uuid>(request, "gate");
  const Point position = getPointArg(request, "position");
  const Angle rotation = getAngleArg(request, "rotation");
  const bool mirror = getArg<bool>(request, "mirror");

  ComponentInstance* cmpInstance =
      schematic.getProject().getCircuit().getComponentInstanceByUuid(
          cmpInstanceUuid);
  if (!cmpInstance) {
    throw RpcInvalidArgs(QString("Unknown component instance '%1'.")
                             .arg(cmpInstanceUuid.toStr()));
  }

  SI_Symbol* symbol = nullptr;
  undoStack.beginCmdGroup(tr("Add symbol to schematic"));  // can throw
  try {
    auto cmdAddSym = new CmdAddSymbolToSchematic(
        editor.getProjectEditor().getWorkspace(), schematic, *cmpInstance,
        gateUuid, position);
    undoStack.appendToCmdGroup(cmdAddSym);  // executes immediately
    symbol = cmdAddSym->getSymbolInstance();

    auto cmdEditSym = new CmdSymbolInstanceEditAll(*symbol);
    cmdEditSym->setRotation(rotation, true);
    cmdEditSym->setMirrored(mirror, true);
    undoStack.appendToCmdGroup(cmdEditSym);

    undoStack.commitCmdGroup();
  } catch (...) {
    try {
      undoStack.abortCmdGroup();
    } catch (...) {
    }
    throw;
  }

  request.send(details(*symbol, registry));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
