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
#include "workspacedbrpc.h"

#include "../../guiapplication.h"
#include "../../library/libraryelementcache.h"
#include "../rpcnotification.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpchelpers.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/org/organization.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/sqlitedatabase.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

template <typename T, typename U>
std::shared_ptr<const T> get(const LibraryElementCache& cache, const U& id) {
  return cache.get<T>(id, true);
}

// template <>
// std::shared_ptr<const Library> get(const LibraryElementCache&, const Uuid&) {
//   throw RpcInvalidArgs("");
// }
//
// template <>
// std::shared_ptr<const Library> get(const LibraryElementCache&, const
// FilePath& fp) {
//   throw RpcInternalError();
// }

static QVariantMap details(GuiApplication& app, const LibraryBaseElement& obj) {
  return {
      {"uuid", toVariant(obj.getUuid())},
      {"version", obj.getVersion().toStr()},
      {"author", obj.getAuthor()},
      {"created", obj.getCreated().toString(Qt::ISODate)},
      {"deprecated", obj.isDeprecated()},
      {"name", *obj.getNames().getDefaultValue()},
      {"description", obj.getDescriptions().getDefaultValue()},
      {"keywords", obj.getKeywords().getDefaultValue()},
      {"path",
       toPrivacyPath(obj.getDirectory().getAbsPath(),
                     app.getWorkspace().getLibrariesPath())},
  };
}

static QVariantMap details(GuiApplication& app, const LibraryCategory& obj) {
  QVariantMap result =
      details(app, static_cast<const LibraryBaseElement&>(obj));
  result.insert({
      {"parent", toVariant(obj.getParentUuid())},
  });
  return result;
}

static QVariantMap details(GuiApplication& app, const LibraryElement& obj) {
  const auto categories = toVariant(
      obj.getCategories(), [](const Uuid& obj) { return toVariant(obj); });
  const auto resources = toVariant(
      obj.getResources(), [](const Resource& obj) { return toVariant(obj); });

  QVariantMap result =
      details(app, static_cast<const LibraryBaseElement&>(obj));
  result.insert({
      {"generated_by", obj.getGeneratedBy()},
      {"categories", categories},
      {"resources", resources},
  });
  return result;
}

static QVariantMap details(GuiApplication& app, const Symbol& obj) {
  const auto pins = toVariant(
      obj.getPins(), [](const SymbolPin& obj) { return toVariant(obj); });
  const auto polygons = toVariant(
      obj.getPolygons(), [](const Polygon& obj) { return toVariant(obj); });
  const auto circles = toVariant(
      obj.getCircles(), [](const Circle& obj) { return toVariant(obj); });
  const auto texts =
      toVariant(obj.getTexts(), [](const Text& obj) { return toVariant(obj); });

  QVariantMap result = details(app, static_cast<const LibraryElement&>(obj));
  result.insert({
      {"grid_interval", obj.getGridInterval()->toMm()},
      {"pins", pins},
      {"polygons", polygons},
      {"circles", circles},
      {"texts", texts},
  });
  return result;
}

static QVariantMap details(GuiApplication& app, const Package& obj) {
  const auto alternativeNames = toVariant(
      obj.getAlternativeNames(), [](const Package::AlternativeName& obj) {
        return QVariantMap{
            {"name", *obj.name},
            {"reference", *obj.reference},
        };
      });
  const auto pads = toVariant(obj.getPads(), [](const PackagePad& obj) {
    return QVariantMap{
        {"uuid", toVariant(obj.getUuid())},
        {"name", *obj.getName()},
    };
  });
  const auto footprints =
      toVariant(obj.getFootprints(), [](const Footprint& obj) {
        const auto tags =
            toVariant(obj.getTags(), [](const Tag& obj) { return *obj; });
        const auto pads = toVariant(obj.getPads(), [](const FootprintPad& obj) {
          return toVariant(obj);
        });
        const auto polygons =
            toVariant(obj.getPolygons(),
                      [](const Polygon& obj) { return toVariant(obj); });
        const auto circles = toVariant(
            obj.getCircles(), [](const Circle& obj) { return toVariant(obj); });
        const auto strokeTexts =
            toVariant(obj.getStrokeTexts(),
                      [](const StrokeText& obj) { return toVariant(obj); });
        const auto zones = toVariant(
            obj.getZones(), [](const Zone& obj) { return toVariant(obj); });
        const auto holes = toVariant(
            obj.getHoles(), [](const Hole& obj) { return toVariant(obj); });

        return QVariantMap{
            {"uuid", toVariant(obj.getUuid())},
            {"name", *obj.getNames().getDefaultValue()},
            {"description", obj.getDescriptions().getDefaultValue()},
            {"tags", tags},
            {"pads", pads},
            {"polygons", polygons},
            {"circles", circles},
            {"stroke_texts", strokeTexts},
            {"zones", zones},
            {"holes", holes},
        };
      });

  QVariantMap result = details(app, static_cast<const LibraryElement&>(obj));
  result.insert({
      {"alternative_names", alternativeNames},
      {"assembly_type", serialize(obj.getAssemblyType(false))->getValue()},
      {"grid_interval", obj.getGridInterval()->toMm()},
      {"min_copper_clearance", toVariant(*obj.getMinCopperClearance())},
      {"pads", pads},
      {"footprints", footprints},
  });
  return result;
}

static QVariantMap details(GuiApplication& app, const Component& obj) {
  const auto prefixes = [](const auto& map) {
    QVariantMap result;
    for (auto it = map.begin(); it != map.end(); it++) {
      result.insert(it.key(), *it.value());
    }
    return result;
  }(obj.getPrefixes().values());
  const auto attributes =
      toVariant(obj.getAttributes(),
                [&](const Attribute& obj) { return toVariant(obj); });
  const auto cmpSignals =
      toVariant(obj.getSignals(), [](const ComponentSignal& obj) {
        return QVariantMap{
            {"uuid", toVariant(obj.getUuid())},
            {"name", *obj.getName()},
            {"role", serialize(obj.getRole())->getValue()},
            {"forced_net_name", obj.getForcedNetName()},
            {"required", obj.isRequired()},
        };
      });
  const auto representations =
      toVariant(obj.getSymbolVariants(), [](const ComponentSymbolVariant& obj) {
        const auto gates = toVariant(
            obj.getSymbolItems(), [&](const ComponentSymbolVariantItem& obj) {
              const auto pinout =
                  toVariant(obj.getPinSignalMap(),
                            [&](const ComponentPinSignalMapItem& obj) {
                              return QVariantMap{
                                  {"pin", toVariant(obj.getPinUuid())},
                                  {"signal", toVariant(obj.getSignalUuid())},
                                  {"display",
                                   serialize(obj.getDisplayType())->getValue()},
                              };
                            });
              return QVariantMap{
                  {"uuid", toVariant(obj.getUuid())},
                  {"symbol", toVariant(obj.getSymbolUuid())},
                  {"position", toVariant(obj.getSymbolPosition())},
                  {"rotation", toVariant(obj.getSymbolRotation())},
                  {"required", obj.isRequired()},
                  {"suffix", *obj.getSuffix()},
                  {"pinout", pinout},
              };
            });
        return QVariantMap{
            {"uuid", toVariant(obj.getUuid())},
            {"norm", obj.getNorm()},
            {"name", *obj.getNames().getDefaultValue()},
            {"description", obj.getDescriptions().getDefaultValue()},
            {"gates", gates},
        };
      });

  QVariantMap result = details(app, static_cast<const LibraryElement&>(obj));
  result.insert({
      {"schematic_only", obj.isSchematicOnly()},
      {"default_value", obj.getDefaultValue()},
      {"prefixes", prefixes},
      {"attributes", attributes},
      {"signals", cmpSignals},
      {"representations", representations},
  });
  return result;
}

static QVariantMap details(GuiApplication& app, const Device& obj) {
  const auto pinout =
      toVariant(obj.getPadSignalMap(), [&](const DevicePadSignalMapItem& obj) {
        return QVariantMap{
            {"pad", toVariant(obj.getPadUuid())},
            {"signal", toVariant(obj.getSignalUuid())},
            {"optional", obj.isOptional()},
        };
      });
  const auto attributes =
      toVariant(obj.getAttributes(),
                [&](const Attribute& obj) { return toVariant(obj); });
  const auto parts = toVariant(obj.getParts(), [&](const Part& obj) {
    const auto attributes =
        toVariant(obj.getAttributes(),
                  [&](const Attribute& obj) { return toVariant(obj); });
    return QVariantMap{
        {"mpn", *obj.getMpn()},
        {"manufacturer", *obj.getManufacturer()},
        {"attributes", attributes},
    };
  });

  QVariantMap result = details(app, static_cast<const LibraryElement&>(obj));
  result.insert({
      {"component", toVariant(obj.getComponentUuid())},
      {"package", toVariant(obj.getPackageUuid())},
      {"pinout", pinout},
      {"attributes", attributes},
      {"parts", parts},
  });
  return result;
}

template <typename T>
static QVariantMap details(GuiApplication& app, const std::optional<Uuid>& uuid,
                           const std::optional<QString>& path) {
  std::shared_ptr<const T> element;
  if (path && (!uuid)) {
    const FilePath fp =
        parseFilePath(*path, app.getWorkspace().getLibrariesPath());
    element = get<T>(app.getLibraryElementCache(), fp);
  } else if (uuid && (!path)) {
    element = get<T>(app.getLibraryElementCache(), *uuid);
  } else {
    throw RpcInvalidArgs("Either 'path' or 'uuid' must be provided.");
  }
  return details(app, *element);
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void WorkspaceDbRpc::wait(GuiApplication& app, RpcObjectRegistry& registry,
                          RpcRequest& request) {
  Q_UNUSED(registry);
  WorkspaceLibraryDb& db = app.getWorkspace().getLibraryDb();
  if (!db.isScanInProgress()) {
    request.send();
    return;
  }

  request.makeAsync(db);
  request.send(RpcNotification().progress(db.getScanProgressPercent()));
  QObject::connect(&db, &WorkspaceLibraryDb::scanProgressUpdate, &request,
                   [&request](int percent) {
                     request.send(RpcNotification().progress(percent));
                   });
  QObject::connect(&db, &WorkspaceLibraryDb::scanFinished, &request,
                   [&request]() { request.send(); });
  QObject::connect(&request, &RpcRequest::cancelled, &request,
                   [&request]() { request.send(RpcCancelled()); });
}

void WorkspaceDbRpc::query_sql(GuiApplication& app, RpcObjectRegistry& registry,
                               RpcRequest& request) {
  Q_UNUSED(registry);
  const QString queryStr = getArg<QString>(request, "query");

  // IMPORTANT: Open the database in read-only mode!
  SQLiteDatabase db(app.getWorkspace().getLibraryDb().getFilePath(), true);
  QSqlQuery query = db.prepareQuery(queryStr);
  query.setForwardOnly(true);
  db.exec(query);

  QSqlRecord rec = query.record();
  const int columns = rec.count();

  QVariantList rows;
  while (query.next()) {
    QVariantMap values;
    for (int i = 0; i < columns; ++i) {
      values.insert(rec.fieldName(i), query.value(i));
    }
    rows.append(values);
  }

  request.send({{"rows", rows}});
}

void WorkspaceDbRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                         RpcRequest& request) {
  Q_UNUSED(registry);

  const QString type = getArg<QString>(request, "type");
  const auto path = getOptionalArg<QString>(request, "path");
  const auto uuid = getOptionalArg<Uuid>(request, "uuid");

  if (type == "component_category") {
    request.send(details<ComponentCategory>(app, uuid, path));
  } else if (type == "package_category") {
    request.send(details<PackageCategory>(app, uuid, path));
  } else if (type == "symbol") {
    request.send(details<Symbol>(app, uuid, path));
  } else if (type == "package") {
    request.send(details<Package>(app, uuid, path));
  } else if (type == "component") {
    request.send(details<Component>(app, uuid, path));
  } else if (type == "device") {
    request.send(details<Device>(app, uuid, path));
  } else if (type == "organization") {
    request.send(details<Organization>(app, uuid, path));
    //} else if (type == "library") {
    //  request.send(details<Library>(app, uuid, fp));
  } else {
    throw RpcInvalidArgs(
        QString("Unknown library element type '%1'.").arg(type));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
