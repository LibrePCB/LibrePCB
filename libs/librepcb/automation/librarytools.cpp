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
#include "librarytools.h"

#include "designsession.h"
#include "mcpserver.h"
#include "toolhelpers.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/network/apiendpoint.h>
#include <librepcb/core/utils/transform.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/library/librarydownload.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Helpers
 ******************************************************************************/

template <typename ElementType>
static QString nameOf(const WorkspaceLibraryDb& db, const FilePath& fp) {
  QString name;
  db.getTranslations<ElementType>(fp, {}, &name);  // can throw
  return name;
}

/**
 * @brief Search elements, sorted by name
 *
 * @return One page of results and the total number of matches
 */
template <typename ElementType>
static std::pair<QJsonArray, int> searchElements(const WorkspaceLibraryDb& db,
                                                 const QString& query,
                                                 int offset, int limit) {
  QList<Uuid> uuids = findLibraryElements<ElementType>(db, query);  // can throw
  if constexpr (std::is_same_v<ElementType, Device>) {
    const auto partDevices = db.findDevicesOfParts(query);  // can throw
    for (const Uuid& uuid : partDevices) {
      if (!uuids.contains(uuid)) uuids.append(uuid);
    }
  }
  QList<std::tuple<QString, Uuid, FilePath>> matches;
  for (const Uuid& uuid : uuids) {
    const FilePath fp = db.getLatest<ElementType>(uuid);  // can throw
    if (fp.isValid()) {
      matches.append(std::make_tuple(nameOf<ElementType>(db, fp), uuid, fp));
    }
  }
  std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
    return std::get<0>(a).compare(std::get<0>(b), Qt::CaseInsensitive) < 0;
  });
  QJsonArray results;
  const auto page = matches.mid(offset, limit);
  for (const auto& [name, uuid, fp] : page) {
    QString description;
    db.getTranslations<ElementType>(fp, {}, nullptr, &description);
    QJsonObject obj{
        {"uuid", uuid.toStr()},
        {"name", name},
        {"description", description.left(200)},
    };
    if constexpr (std::is_same_v<ElementType, Device>) {
      Uuid cmpUuid = uuid, pkgUuid = uuid;
      db.getDeviceMetadata(fp, &cmpUuid, &pkgUuid);  // can throw
      obj.insert("component",
                 nameOf<Component>(db, db.getLatest<Component>(cmpUuid)));
      obj.insert("package",
                 nameOf<Package>(db, db.getLatest<Package>(pkgUuid)));
    }
    results.append(obj);
  }
  return std::make_pair(results, int(matches.count()));
}

static QList<ApiEndpoint::Library> fetchRemoteLibraries(Workspace& ws) {
  QList<ApiEndpoint::Library> result;
  QStringList errors;
  QList<std::shared_ptr<ApiEndpoint>> pending;
  QEventLoop loop;
  for (const WorkspaceSettings::ApiEndpoint& ep :
       ws.getSettings().apiEndpoints.get()) {
    if ((!ep.url.isValid()) || (!ep.useForLibraries)) continue;
    auto endpoint = std::make_shared<ApiEndpoint>(ep.url);
    auto done = [&pending, &loop, ptr = endpoint.get()]() {
      pending.removeIf([ptr](const auto& e) { return e.get() == ptr; });
      if (pending.isEmpty()) loop.quit();
    };
    QObject::connect(endpoint.get(), &ApiEndpoint::libraryListReceived, &loop,
                     [&result, done](QList<ApiEndpoint::Library> libs) {
                       result += libs;
                       done();
                     });
    QObject::connect(endpoint.get(),
                     &ApiEndpoint::errorWhileFetchingLibraryList, &loop,
                     [&errors, done](const QString& msg) {
                       errors.append(msg);
                       done();
                     });
    pending.append(endpoint);
    endpoint->requestLibraryList(true);
  }
  if (!pending.isEmpty()) {
    QTimer::singleShot(120000, &loop, &QEventLoop::quit);
    loop.exec();
  }
  if (!pending.isEmpty()) {
    errors.append("Timeout while waiting for the API server.");
  }
  if (!errors.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to fetch the library list: " % errors.join(" "));
  }
  return result;
}

static FilePath findLocalLibrary(Workspace& ws, const QString& nameOrUuid) {
  const FilePath dir =
      findLibraryElement<Library>(ws.getLibraryDb(), nameOrUuid).second;
  if (!dir.isLocatedInDir(ws.getLocalLibrariesPath())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The library '%1' is not a local library, thus it cannot "
                "be modified.")
            .arg(nameOrUuid));
  }
  return dir;
}

template <typename ElementType>
static QSet<Uuid> findCategories(const WorkspaceLibraryDb& db,
                                 const QJsonValue& value) {
  QSet<Uuid> uuids;
  const QJsonArray categoryValues = value.toArray();
  for (const QJsonValue& category : categoryValues) {
    uuids.insert(
        findLibraryElement<ElementType>(db, category.toString()).first);
  }
  return uuids;
}

template <typename ElementType>
static QJsonObject saveNewElement(DesignSession& session, ElementType& element,
                                  const FilePath& libDir) {
  const FilePath dir = libDir.getPathTo(ElementType::getShortElementName())
                           .getPathTo(element.getUuid().toStr());
  auto fs = TransactionalFileSystem::openRW(dir);  // can throw
  TransactionalDirectory td(fs);
  element.moveTo(td);  // can throw
  fs->save();  // can throw
  session.rescanLibraries();  // can throw
  QJsonObject result = toJson(element.runChecks(),  // can throw
                              element.getMessageApprovals());
  result.insert("uuid", element.getUuid().toStr());
  result.insert("name", *element.getNames().getDefaultValue());
  return result;
}

static ElementName elementName(const QJsonObject& args) {
  return ElementName(cleanElementName(requiredString(args, "name")));
}

static Version initialVersion() noexcept {
  return Version::fromString("0.1");
}

/*******************************************************************************
 *  Library Element Details
 ******************************************************************************/

static QJsonObject symbolDetails(const Symbol& sym) noexcept {
  QJsonArray pins;
  for (const SymbolPin& pin : sym.getPins()) {
    pins.append(QJsonObject{
        {"name", *pin.getName()},
        {"position", toJson(pin.getPosition())},
        {"rotation", pin.getRotation().toDeg()},
        {"length", pin.getLength()->toMm()},
    });
  }
  return QJsonObject{{"pins", pins}};
}

static QJsonObject componentDetails(const WorkspaceLibraryDb& db,
                                    const Component& cmp) {
  QJsonArray signalArray;
  for (const ComponentSignal& sig : cmp.getSignals()) {
    signalArray.append(*sig.getName());
  }
  QJsonArray variants;
  for (const ComponentSymbolVariant& var : cmp.getSymbolVariants()) {
    variants.append(QJsonObject{
        {"name", *var.getNames().getDefaultValue()},
        {"gateCount", var.getSymbolItems().count()},
    });
  }
  QJsonArray attributes;
  for (const Attribute& attr : cmp.getAttributes()) {
    attributes.append(QJsonObject{
        {"key", *attr.getKey()},
        {"value", attr.getValue()},
        {"type", attr.getType().getName()},
    });
  }
  QJsonArray devices;
  const auto deviceUuids = db.getComponentDevices(cmp.getUuid());
  for (const Uuid& uuid : deviceUuids) {
    const FilePath fp = db.getLatest<Device>(uuid);  // can throw
    if (!fp.isValid()) continue;
    devices.append(QJsonObject{
        {"uuid", uuid.toStr()},
        {"name", nameOf<Device>(db, fp)},
    });
  }
  return QJsonObject{
      {"prefix", *cmp.getPrefixes().getDefaultValue()},
      {"defaultValue", cmp.getDefaultValue()},
      {"schematicOnly", cmp.isSchematicOnly()},
      {"signals", signalArray},
      {"symbolVariants", variants},
      {"attributes", attributes},
      {"devices", devices},
  };
}

static QString shapeName(const FootprintPad& pad) noexcept {
  switch (pad.getShape()) {
    case Pad::Shape::RoundedOctagon:
      return "octagon";
    case Pad::Shape::Custom:
      return "custom";
    default:
      break;
  }
  if (*pad.getRadius() == Ratio::fromPercent(0)) {
    return "rect";
  } else if (*pad.getRadius() == Ratio::fromPercent(100)) {
    return "round";
  }
  return "roundrect";
}

/**
 * @brief Pads, footprints and the geometry of one footprint (default: first)
 */
static QJsonObject packageDetails(const Package& pkg,
                                  const QString& footprintName) {
  QJsonArray pads;
  for (const PackagePad& pad : pkg.getPads()) {
    pads.append(*pad.getName());
  }
  QJsonArray footprints;
  std::shared_ptr<const Footprint> footprint = pkg.getFootprints().value(0);
  for (int i = 0; i < pkg.getFootprints().count(); ++i) {
    std::shared_ptr<const Footprint> fpt = pkg.getFootprints().value(i);
    footprints.append(*fpt->getNames().getDefaultValue());
    if (fpt->getNames().getDefaultValue()->compare(footprintName,
                                                   Qt::CaseInsensitive) == 0) {
      footprint = fpt;
    }
  }
  if ((!footprintName.isEmpty()) &&
      ((!footprint) ||
       (footprint->getNames().getDefaultValue()->compare(
            footprintName, Qt::CaseInsensitive) != 0))) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Footprint '%1' not found. Available: %2")
            .arg(footprintName,
                 QJsonDocument(footprints).toJson(QJsonDocument::Compact)));
  }
  QJsonArray footprintPads;
  QJsonObject result;
  if (footprint) {
    result = footprintGeometry(*footprint, Transform());
    result.insert("footprint", *footprint->getNames().getDefaultValue());
    for (const FootprintPad& pad : footprint->getPads()) {
      std::shared_ptr<const PackagePad> pkgPad = pad.getPackagePadUuid()
          ? pkg.getPads().find(*pad.getPackagePadUuid())
          : nullptr;
      const std::shared_ptr<const PadHole> hole = pad.getHoles().value(0);
      QJsonObject obj{
          {"name", pkgPad ? QJsonValue(*pkgPad->getName()) : QJsonValue()},
          {"position", toJson(pad.getPosition())},
          {"rotation", pad.getRotation().toDeg()},
          {"size", QJsonArray{pad.getWidth()->toMm(), pad.getHeight()->toMm()}},
          {"shape", shapeName(pad)},
          {"drill",
           hole ? QJsonValue(hole->getDiameter()->toMm()) : QJsonValue()},
          {"side",
           pad.isTht() ? "both"
                       : ((pad.getComponentSide() == Pad::ComponentSide::Bottom)
                              ? "bottom"
                              : "top")},
      };
      if (hole && hole->isSlot()) {
        obj.insert(
            "slotLength",
            (*hole->getPath()->getTotalStraightLength() + *hole->getDiameter())
                .toMm());
      }
      footprintPads.append(obj);
    }
  }
  result.insert("pads", pads);
  result.insert("footprints", footprints);
  result.insert("footprintPads", footprintPads);
  return result;
}

static QJsonObject deviceDetails(const WorkspaceLibraryDb& db,
                                 const Device& dev,
                                 const QString& footprintName) {
  const FilePath cmpFp = db.getLatest<Component>(dev.getComponentUuid());
  const FilePath pkgFp = db.getLatest<Package>(dev.getPackageUuid());
  if ((!cmpFp.isValid()) || (!pkgFp.isValid())) {
    throw RuntimeError(__FILE__, __LINE__,
                       "The component or package of the device is missing.");
  }
  std::unique_ptr<Component> cmp = openElement<Component>(cmpFp);
  std::unique_ptr<Package> pkg = openElement<Package>(pkgFp);
  QJsonObject padMap;
  for (const DevicePadSignalMapItem& item : dev.getPadSignalMap()) {
    const PackagePad* pad = pkg->getPads().find(item.getPadUuid()).get();
    const ComponentSignal* sig = item.getSignalUuid()
        ? cmp->getSignals().find(*item.getSignalUuid()).get()
        : nullptr;
    if (pad) {
      padMap.insert(*pad->getName(),
                    sig ? QJsonValue(*sig->getName()) : QJsonValue());
    }
  }
  QJsonArray parts;
  for (const Part& part : dev.getParts()) {
    parts.append(QJsonObject{
        {"mpn", *part.getMpn()},
        {"manufacturer", *part.getManufacturer()},
    });
  }
  QJsonObject result = packageDetails(*pkg, footprintName);
  result.insert("component", *cmp->getNames().getDefaultValue());
  result.insert("package", *pkg->getNames().getDefaultValue());
  result.insert("padSignalMap", padMap);
  result.insert("parts", parts);
  return result;
}

/*******************************************************************************
 *  Functions
 ******************************************************************************/

void addLibraryTools(McpServer& server, DesignSession& session) noexcept {
  const QStringList types = {
      "component",         "device",         "symbol", "package",
      "componentCategory", "packageCategory"};

  server.addTool(McpTool{
      "workspace_open",
      "Open a LibrePCB workspace, or create a new one if the directory does "
      "not exist or is empty. The workspace contains the libraries and is "
      "required for all library tools and for adding parts to a project.",
      objectSchema(
          {{"path", stringProp("Absolute path to the workspace directory")},
           {"userName",
            stringProp("Optional user name, used as "
                       "author of new elements")}},
          {"path"}),
      false,
      [&session](const QJsonObject& args) {
        session.openWorkspace(absoluteFilePath(requiredString(args, "path")));
        Workspace& ws = session.getWorkspace();
        const QString userName = args.value("userName").toString().trimmed();
        if (!userName.isEmpty()) {
          ws.getSettings().userName.set(userName);
          ws.saveSettings();  // can throw
        }
        QJsonArray libraries;
        const auto libs =
            ws.getLibraryDb().getAll<Library>(std::nullopt, FilePath());
        for (const FilePath& fp : libs) {
          libraries.append(nameOf<Library>(ws.getLibraryDb(), fp));
        }
        return McpToolResult::json(QJsonObject{
            {"path", ws.getPath().toNative()},
            {"installedLibraries", libraries},
        });
      },
  });

  server.addTool(McpTool{
      "library_list_remote",
      "List the libraries available for download from the LibrePCB API "
      "server.",
      QJsonObject(),
      true,
      [&session](const QJsonObject&) {
        Workspace& ws = session.getWorkspace();
        session.startNetworking();
        QJsonArray array;
        const auto libraries = fetchRemoteLibraries(ws);
        for (const ApiEndpoint::Library& lib : libraries) {
          array.append(QJsonObject{
              {"name", lib.name},
              {"description", lib.description},
              {"recommended", lib.recommended},
              {"installed",
               ws.getRemoteLibrariesPath()
                   .getPathTo(lib.uuid.toStr() % ".lplib")
                   .isExistingDir()},
          });
        }
        return McpToolResult::json(QJsonObject{{"libraries", array}});
      },
  });

  server.addTool(McpTool{
      "library_install",
      "Download and install (or update) libraries from the LibrePCB API "
      "server, including their dependencies.",
      objectSchema(
          {{"names",
            arrayProp(stringProp("Library name"),
                      "Names of the libraries, e.g. 'LibrePCB Base'")}},
          {"names"}),
      false,
      [&session](const QJsonObject& args) {
        Workspace& ws = session.getWorkspace();
        session.startNetworking();
        const QList<ApiEndpoint::Library> remoteLibs = fetchRemoteLibraries(ws);
        QList<ApiEndpoint::Library> toInstall;
        std::function<void(const ApiEndpoint::Library&)> add =
            [&](const ApiEndpoint::Library& lib) {
              for (const ApiEndpoint::Library& l : toInstall) {
                if (l.uuid == lib.uuid) return;
              }
              toInstall.append(lib);
              for (const Uuid& dep : lib.dependencies) {
                for (const ApiEndpoint::Library& l : remoteLibs) {
                  if (l.uuid == dep) add(l);
                }
              }
            };
        const QJsonArray nameValues = args.value("names").toArray();
        for (const QJsonValue& name : nameValues) {
          bool found = false;
          for (const ApiEndpoint::Library& lib : remoteLibs) {
            if (lib.name.compare(name.toString(), Qt::CaseInsensitive) == 0) {
              add(lib);
              found = true;
            }
          }
          if (!found) {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("No remote library named '%1' found.")
                                   .arg(name.toString()));
          }
        }

        QStringList errors;
        QStringList installed;
        int pending = 0;
        QEventLoop loop;
        auto semaphore = std::make_shared<QSemaphore>(4);
        QList<std::shared_ptr<editor::LibraryDownload>> downloads;
        for (const ApiEndpoint::Library& lib : toInstall) {
          const FilePath destDir = ws.getRemoteLibrariesPath().getPathTo(
              lib.uuid.toStr() % ".lplib");
          auto dl = std::make_shared<editor::LibraryDownload>(
              lib.downloadUrl, destDir, semaphore);
          if (lib.downloadSize > 0) {
            dl->setExpectedZipFileSize(lib.downloadSize);
          }
          if (!lib.downloadSha256.isEmpty()) {
            dl->setExpectedChecksum(QCryptographicHash::Sha256,
                                    QByteArray::fromHex(lib.downloadSha256));
          }
          if (destDir.isExistingDir()) {
            dl->setExistingDirsToReplace({destDir});
          }
          QObject::connect(
              dl.get(), &editor::LibraryDownload::finished, &loop,
              [&, name = lib.name](bool success, const QString& errMsg) {
                if (success) {
                  installed.append(name);
                } else {
                  errors.append(name % ": " % errMsg);
                }
                if (--pending == 0) loop.quit();
              },
              Qt::QueuedConnection);
          downloads.append(dl);
          ++pending;
          dl->start();
        }
        if (pending > 0) {
          loop.exec();
        }
        session.rescanLibraries();  // can throw
        if (!errors.isEmpty()) {
          throw RuntimeError(__FILE__, __LINE__,
                             "Download failed: " % errors.join("; "));
        }
        return McpToolResult::json(QJsonObject{
            {"installed", QJsonArray::fromStringList(installed)},
        });
      },
  });

  server.addTool(McpTool{
      "library_search",
      "Search the installed libraries by keyword (case-insensitive substring "
      "of name, description or keywords; devices also by part number). An "
      "empty query lists all elements. Results are sorted by name.",
      objectSchema(
          {{"query", stringProp("Keyword, e.g. 'resistor', or '' for all")},
           {"type", enumProp(types, "Type of library element")},
           {"offset",
            QJsonObject{
                {"type", "integer"},
                {"minimum", 0},
                {"description", "Number of results to skip, default 0"}}},
           {"limit",
            QJsonObject{
                {"type", "integer"},
                {"minimum", 1},
                {"description", "Maximum number of results, default 50"}}}},
          {"query", "type"}),
      true,
      [&session, types](const QJsonObject& args) {
        const WorkspaceLibraryDb& db = session.getWorkspace().getLibraryDb();
        const QString query = args.value("query").toString().trimmed();
        const QString type = requiredString(args, "type");
        const int offset = std::max(args.value("offset").toInt(0), 0);
        const int limit = std::max(args.value("limit").toInt(50), 1);
        std::pair<QJsonArray, int> results;
        if (type == "component") {
          results = searchElements<Component>(db, query, offset, limit);
        } else if (type == "device") {
          results = searchElements<Device>(db, query, offset, limit);
        } else if (type == "symbol") {
          results = searchElements<Symbol>(db, query, offset, limit);
        } else if (type == "package") {
          results = searchElements<Package>(db, query, offset, limit);
        } else if (type == "componentCategory") {
          results = searchElements<ComponentCategory>(db, query, offset, limit);
        } else if (type == "packageCategory") {
          results = searchElements<PackageCategory>(db, query, offset, limit);
        } else {
          throw RuntimeError(
              __FILE__, __LINE__,
              "Invalid type, must be one of: " % types.join(", "));
        }
        return McpToolResult::json(QJsonObject{
            {"results", results.first},
            {"total", results.second},
        });
      },
  });

  server.addTool(McpTool{
      "library_get",
      "Get details of a library element: pins of a symbol; signals, symbol "
      "variants, attributes and devices of a component; pads, footprints, "
      "pad geometry, courtyard, keep-outs and holes (of one footprint, "
      "relative to the origin) of a package; additionally pad-signal-map and "
      "parts of a device.",
      objectSchema(
          {{"type",
            enumProp({"component", "device", "symbol", "package"},
                     "Type of library element")},
           {"element", stringProp("Name or UUID of the element")},
           {"footprint",
            stringProp("Footprint of a package or device, default: the "
                       "first one")}},
          {"type", "element"}),
      true,
      [&session](const QJsonObject& args) {
        const WorkspaceLibraryDb& db = session.getWorkspace().getLibraryDb();
        const QString type = requiredString(args, "type");
        const QString element = requiredString(args, "element");
        QJsonObject result;
        std::pair<Uuid, FilePath> found(Uuid::createRandom(), FilePath());
        if (type == "component") {
          found = findLibraryElement<Component>(db, element);
          result = componentDetails(db, *openElement<Component>(found.second));
        } else if (type == "device") {
          found = findLibraryElement<Device>(db, element);
          result = deviceDetails(db, *openElement<Device>(found.second),
                                 args.value("footprint").toString());
        } else if (type == "symbol") {
          found = findLibraryElement<Symbol>(db, element);
          result = symbolDetails(*openElement<Symbol>(found.second));
        } else if (type == "package") {
          found = findLibraryElement<Package>(db, element);
          result = packageDetails(*openElement<Package>(found.second),
                                  args.value("footprint").toString());
        } else {
          throw RuntimeError(__FILE__, __LINE__, "Invalid type.");
        }
        result.insert("uuid", found.first.toStr());
        return McpToolResult::json(result);
      },
  });

  server.addTool(McpTool{
      "library_create",
      "Create a new local library in the workspace for your own parts.",
      objectSchema({{"name", stringProp("Library name, e.g. 'Custom Parts'")},
                    {"description", stringProp("Optional description")},
                    {"dependencies",
                     arrayProp(stringProp("Library name"),
                               "Libraries whose elements are used, e.g. "
                               "['LibrePCB Base']")}},
                   {"name"}),
      false,
      [&session](const QJsonObject& args) {
        Workspace& ws = session.getWorkspace();
        const ElementName name = elementName(args);
        const FilePath dir = ws.getLocalLibrariesPath().getPathTo(
            FilePath::cleanFileName(
                *name, FilePath::ReplaceSpaces | FilePath::KeepCase) %
            ".lplib");
        if (dir.isExistingDir() || dir.isExistingFile()) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("The directory '%1' exists already.")
                                 .arg(dir.toNative()));
        }
        QSet<Uuid> dependencies;
        const QJsonArray dependencyValues =
            args.value("dependencies").toArray();
        for (const QJsonValue& dep : dependencyValues) {
          dependencies.insert(
              findLibraryElement<Library>(ws.getLibraryDb(), dep.toString())
                  .first);
        }
        Library lib(Uuid::createRandom(), initialVersion(),
                    ws.getSettings().userName.get(),
                    QDateTime::currentDateTime(), name,
                    args.value("description").toString(), QString());
        lib.setDependencies(dependencies);
        auto fs = TransactionalFileSystem::openRW(dir);  // can throw
        TransactionalDirectory td(fs);
        lib.moveTo(td);  // can throw
        fs->save();  // can throw
        session.rescanLibraries();  // can throw
        return McpToolResult::json(QJsonObject{
            {"uuid", lib.getUuid().toStr()},
            {"path", dir.toNative()},
        });
      },
  });

  const QJsonObject pinSchema = objectSchema(
      {{"name", stringProp("Pin name, e.g. 'VCC'")},
       {"position",
        pointProp("Outer end of the pin where wires connect, on "
                  "the 2.54mm grid")},
       {"rotation",
        numberProp("Direction from the outer end towards the symbol body in "
                   "degrees: 0 = to the right (pin on the left side), 180 = "
                   "to the left (pin on the right side), 90 = up, 270 = "
                   "down")},
       {"length", numberProp("Pin length in mm, default 2.54")}},
      {"name", "position", "rotation"});

  server.addTool(McpTool{
      "symbol_create",
      "Create a schematic symbol with pins and a rectangular body in a local "
      "library. Name and value texts are added automatically above and "
      "below the body. Returns the result of the symbol check.",
      objectSchema(
          {{"library", stringProp("Name of the local library")},
           {"name", stringProp("Symbol name, e.g. 'LM317'")},
           {"description", stringProp("Optional description")},
           {"categories",
            arrayProp(stringProp("Name or UUID"), "Component categories")},
           {"pins", arrayProp(pinSchema, "The pins")},
           {"bodyFrom", pointProp("One corner of the body rectangle")},
           {"bodyTo", pointProp("Opposite corner of the body rectangle")}},
          {"library", "name", "pins", "bodyFrom", "bodyTo"}),
      false,
      [&session](const QJsonObject& args) {
        Workspace& ws = session.getWorkspace();
        const FilePath libDir =
            findLocalLibrary(ws, requiredString(args, "library"));
        Symbol sym(Uuid::createRandom(), initialVersion(),
                   ws.getSettings().userName.get(),
                   QDateTime::currentDateTime(), elementName(args),
                   args.value("description").toString(), QString());
        sym.setCategories(findCategories<ComponentCategory>(
            ws.getLibraryDb(), args.value("categories")));
        const QJsonArray pinValues = args.value("pins").toArray();
        for (const QJsonValue& value : pinValues) {
          const QJsonObject pin = value.toObject();
          const QJsonValue lengthValue = pin.value("length");
          const UnsignedLength length(lengthValue.isUndefined()
                                          ? Length(2540000)
                                          : lengthMm(lengthValue, "length"));
          sym.getPins().append(std::make_shared<SymbolPin>(
              Uuid::createRandom(),
              CircuitIdentifier(requiredString(pin, "name")),
              pointMm(pin.value("position"), "position"), length,
              angleDeg(pin.value("rotation"), "rotation"),
              SymbolPin::getDefaultNamePosition(length), Angle::deg0(),
              SymbolPin::getDefaultNameHeight(),
              SymbolPin::getDefaultNameAlignment()));
        }
        const Point p1 = pointMm(args.value("bodyFrom"), "bodyFrom");
        const Point p2 = pointMm(args.value("bodyTo"), "bodyTo");
        const Point topLeft(std::min(p1.getX(), p2.getX()),
                            std::max(p1.getY(), p2.getY()));
        const Point bottomLeft(std::min(p1.getX(), p2.getX()),
                               std::min(p1.getY(), p2.getY()));
        sym.getPolygons().append(std::make_shared<Polygon>(
            Uuid::createRandom(), Layer::symbolOutlines(),
            UnsignedLength(200000), false, true, Path::rect(p1, p2)));
        sym.getTexts().append(std::make_shared<Text>(
            Uuid::createRandom(), Layer::symbolNames(), "{{NAME}}",
            topLeft + Point(0, 1270000), Angle::deg0(), PositiveLength(2500000),
            Alignment(HAlign::left(), VAlign::bottom()), false));
        sym.getTexts().append(std::make_shared<Text>(
            Uuid::createRandom(), Layer::symbolValues(), "{{VALUE}}",
            bottomLeft - Point(0, 1270000), Angle::deg0(),
            PositiveLength(2500000), Alignment(HAlign::left(), VAlign::top()),
            false));
        return McpToolResult::json(saveNewElement(session, sym, libDir));
      },
  });

  const QJsonObject padSchema = objectSchema(
      {{"name", stringProp("Pad name, e.g. '1'")},
       {"position", pointProp("Pad center relative to the package origin")},
       {"size", pointProp("Pad size [width, height]")},
       {"rotation", numberProp("Rotation in degrees")},
       {"shape",
        enumProp({"rect", "roundrect", "round", "octagon", "custom"},
                 "Pad shape, default roundrect for SMT and round for THT "
                 "pads. 'round' with different width and height is an "
                 "oblong pad. 'custom' takes its outline from 'vertices'.")},
       {"vertices",
        verticesProp("Outline of a custom pad, relative to the pad center")},
       {"drill", numberProp("Drill diameter in mm, makes it a plated THT pad")},
       {"slotLength",
        numberProp("Total length of a slotted drill along the pad's x axis, "
                   "'drill' is its width")},
       {"paste", boolProp("Solder paste on an SMT pad, default true")},
       {"side",
        enumProp({"top", "bottom"}, "Side of an SMT pad, default top")}},
      {"name", "position", "size"});

  server.addTool(McpTool{
      "package_create",
      "Create a package with one footprint in a local library: pads, a "
      "package outline from the body size, a courtyard around body and pads, "
      "name/value texts, and optionally holes, keep-outs and a silkscreen "
      "outline with pin 1 marker. The origin should be the center of the "
      "body. Returns the result of the package check.",
      objectSchema(
          {{"library", stringProp("Name of the local library")},
           {"name", stringProp("Package name, e.g. 'QFN-16 3x3'")},
           {"description", stringProp("Optional description")},
           {"categories",
            arrayProp(stringProp("Name or UUID"), "Package categories")},
           {"assemblyType",
            enumProp({"smt", "tht", "mixed", "none", "other"},
                     "Default: derived from the pads; 'none' for footprints "
                     "without a part to mount, e.g. test pads")},
           {"pads", arrayProp(padSchema, "The pads")},
           {"holes",
            arrayProp(objectSchema({{"position", pointProp("Center")},
                                    {"diameter", numberProp("Diameter")}},
                                   {"position", "diameter"}),
                      "Non-plated holes, e.g. for mounting pegs")},
           {"body",
            pointProp("Size [width, height] of the body, centered at the "
                      "origin, default: around the pads")},
           {"courtyardMargin",
            numberProp("Courtyard distance to body and pads, default "
                       "0.25")},
           {"keepouts",
            arrayProp(objectSchema(
                          {{"vertices", verticesProp("Area of the keep-out")},
                           {"layers",
                            enumProp({"top", "bottom", "inner", "all"},
                                     "Copper layers, default top (the side of "
                                     "the part, mirrored on the bottom side)")},
                           {"rules",
                            arrayProp(enumProp(zoneRuleNames(), "Rule"),
                                      "Default ['noCopper', 'noPlanes']")}},
                          {"vertices"}),
                      "Keep-out zones, e.g. below an antenna or a hole")},
           {"legend",
            boolProp("Draw the body outline and a pin 1 marker on the top "
                     "silkscreen, default false")}},
          {"library", "name", "pads"}),
      false,
      [&session](const QJsonObject& args) {
        Workspace& ws = session.getWorkspace();
        const FilePath libDir =
            findLocalLibrary(ws, requiredString(args, "library"));
        auto fpt = std::make_shared<Footprint>(Uuid::createRandom(),
                                               ElementName("default"), "");
        Package pkg(Uuid::createRandom(), initialVersion(),
                    ws.getSettings().userName.get(),
                    QDateTime::currentDateTime(), elementName(args),
                    args.value("description").toString(), QString(),
                    Package::AssemblyType::Other);
        pkg.setCategories(findCategories<PackageCategory>(
            ws.getLibraryDb(), args.value("categories")));
        QRectF bounds;  // In mm, Y up.
        auto extend = [&bounds](const Point& center, qreal w, qreal h) {
          const QRectF rect(center.getX().toMm() - w / 2,
                            center.getY().toMm() - h / 2, w, h);
          bounds = bounds.isNull() ? rect : bounds.united(rect);
        };
        bool hasTht = false, hasSmt = false;
        std::optional<Point> pin1;
        const QJsonArray padValues = args.value("pads").toArray();
        for (const QJsonValue& value : padValues) {
          const QJsonObject obj = value.toObject();
          const CircuitIdentifier name(requiredString(obj, "name"));
          if (pkg.getPads().contains(*name)) {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("Duplicate pad name '%1'.").arg(*name));
          }
          auto pkgPad =
              std::make_shared<PackagePad>(Uuid::createRandom(), name);
          pkg.getPads().append(pkgPad);
          const Point pos = pointMm(obj.value("position"), "position");
          if ((!pin1) || (*name == "1")) pin1 = pos;
          const Point size = pointMm(obj.value("size"), "size");
          const PositiveLength width(size.getX());  // can throw
          const PositiveLength height(size.getY());  // can throw
          const bool tht = obj.contains("drill");
          (tht ? hasTht : hasSmt) = true;
          const QString shape =
              obj.value("shape").toString(tht ? "round" : "roundrect");
          Path customOutline;
          if (shape == "custom") {
            customOutline = closedPathMm(obj.value("vertices"), "vertices");
          } else if (obj.contains("vertices")) {
            throw RuntimeError(__FILE__, __LINE__,
                               "Only custom pads take 'vertices'.");
          }
          UnsignedLimitedRatio radius(Ratio::fromPercent(0));
          if (shape == "round") {
            radius = UnsignedLimitedRatio(Ratio::fromPercent(100));
          } else if (shape == "roundrect") {
            radius = FootprintPad::getRecommendedRadius(width, height);
          }
          PadHoleList holes;
          if (tht) {
            const PositiveLength drill(lengthMm(obj.value("drill"), "drill"));
            Path holePath({Vertex(Point())});
            if (obj.contains("slotLength")) {
              const Length half =
                  (lengthMm(obj.value("slotLength"), "slotLength") - *drill) /
                  2;
              if (half <= 0) {
                throw RuntimeError(__FILE__, __LINE__,
                                   "'slotLength' must be larger than "
                                   "'drill'.");
              }
              holePath =
                  Path({Vertex(Point(-half, 0)), Vertex(Point(half, 0))});
            }
            holes.append(std::make_shared<PadHole>(
                Uuid::createRandom(), drill,
                NonEmptyPath(holePath)));  // can throw
          }
          const MaskConfig paste = (tht || (!obj.value("paste").toBool(true)))
              ? MaskConfig::off()
              : MaskConfig::automatic();
          const Angle rotation = angleDeg(obj.value("rotation"), "rotation");
          fpt->getPads().append(std::make_shared<FootprintPad>(
              Uuid::createRandom(), pkgPad->getUuid(), pos, rotation,
              (shape == "octagon")
                  ? Pad::Shape::RoundedOctagon
                  : ((shape == "custom") ? Pad::Shape::Custom
                                         : Pad::Shape::RoundedRect),
              width, height, radius, customOutline, MaskConfig::automatic(),
              paste, UnsignedLength(0),
              (obj.value("side") == "bottom") ? Pad::ComponentSide::Bottom
                                              : Pad::ComponentSide::Top,
              Pad::Function::StandardPad, holes));
          const QRectF rect =
              ((shape == "custom") ? customOutline
                                   : Path::centeredRect(width, height))
                  .rotated(rotation)
                  .toQPainterPathPx()
                  .boundingRect();
          extend(pos, Length::fromPx(rect.width()).toMm(),
                 Length::fromPx(rect.height()).toMm());
        }
        const QJsonArray holeValues = args.value("holes").toArray();
        for (const QJsonValue& value : holeValues) {
          const QJsonObject obj = value.toObject();
          const Point pos = pointMm(obj.value("position"), "position");
          const PositiveLength diameter(
              lengthMm(obj.value("diameter"), "diameter"));  // can throw
          fpt->getHoles().append(std::make_shared<Hole>(
              Uuid::createRandom(), diameter, makeNonEmptyPath(pos),
              MaskConfig::automatic()));
          extend(pos, diameter->toMm(), diameter->toMm());
        }
        const QString type = args.value("assemblyType")
                                 .toString(hasTht ? (hasSmt ? "mixed" : "tht")
                                                  : (hasSmt ? "smt" : "none"));
        const QHash<QString, Package::AssemblyType> types = {
            {"smt", Package::AssemblyType::Smt},
            {"tht", Package::AssemblyType::Tht},
            {"mixed", Package::AssemblyType::Mixed},
            {"none", Package::AssemblyType::None},
            {"other", Package::AssemblyType::Other},
        };
        pkg.setAssemblyType(types.value(type));

        // Package outline and courtyard.
        QRectF body = bounds;
        if (args.contains("body")) {
          const Point size = pointMm(args.value("body"), "body");
          body = QRectF(-size.getX().toMm() / 2, -size.getY().toMm() / 2,
                        size.getX().toMm(), size.getY().toMm());
        }
        const qreal margin = args.contains("courtyardMargin")
            ? lengthMm(args.value("courtyardMargin"), "courtyardMargin").toMm()
            : 0.25;
        const QRectF courtyard = body.united(bounds).marginsAdded(
            QMarginsF(margin, margin, margin, margin));
        auto rectPath = [](const QRectF& r) {
          return Path::rect(Point::fromMm(r.left(), r.top()),
                            Point::fromMm(r.right(), r.bottom()));
        };
        if (pkg.getAssemblyType(false) != Package::AssemblyType::None) {
          fpt->getPolygons().append(std::make_shared<Polygon>(
              Uuid::createRandom(), Layer::topPackageOutlines(),
              UnsignedLength(0), false, false, rectPath(body)));
          fpt->getPolygons().append(std::make_shared<Polygon>(
              Uuid::createRandom(), Layer::topCourtyard(), UnsignedLength(0),
              false, false, rectPath(courtyard)));
        }
        fpt->getStrokeTexts().append(std::make_shared<StrokeText>(
            Uuid::createRandom(), Layer::topNames(), "{{NAME}}",
            Point::fromMm(0, courtyard.bottom() + 0.2), Angle::deg0(),
            PositiveLength(1000000), UnsignedLength(200000),
            StrokeTextSpacing(), StrokeTextSpacing(),
            Alignment(HAlign::center(), VAlign::bottom()), false, true, false));
        fpt->getStrokeTexts().append(std::make_shared<StrokeText>(
            Uuid::createRandom(), Layer::topValues(), "{{VALUE}}",
            Point::fromMm(0, courtyard.top() - 0.2), Angle::deg0(),
            PositiveLength(1000000), UnsignedLength(200000),
            StrokeTextSpacing(), StrokeTextSpacing(),
            Alignment(HAlign::center(), VAlign::top()), false, true, false));
        const QJsonArray keepoutValues = args.value("keepouts").toArray();
        for (const QJsonValue& value : keepoutValues) {
          const QJsonObject obj = value.toObject();
          const QString layers = obj.value("layers").toString("top");
          Zone::Layers flags;
          if ((layers == "top") || (layers == "all")) {
            flags |= Zone::Layer::Top;
          }
          if ((layers == "inner") || (layers == "all")) {
            flags |= Zone::Layer::Inner;
          }
          if ((layers == "bottom") || (layers == "all")) {
            flags |= Zone::Layer::Bottom;
          }
          fpt->getZones().append(std::make_shared<Zone>(
              Uuid::createRandom(), flags,
              zoneRules(obj.contains("rules")
                            ? obj.value("rules")
                            : QJsonArray{"noCopper", "noPlanes"}),
              closedPathMm(obj.value("vertices"), "vertices")));
        }
        if (args.value("legend").toBool(false)) {
          const qreal width = 0.15;
          const QRectF legend =
              body.marginsAdded(QMarginsF(width, width, width, width));
          fpt->getPolygons().append(std::make_shared<Polygon>(
              Uuid::createRandom(), Layer::topLegend(),
              UnsignedLength(Length::fromMm(width)), false, false,
              rectPath(legend)));
          // A dot outside the outline corner which is closest to pin 1.
          const QPointF p = pin1
              ? QPointF(pin1->getX().toMm(), pin1->getY().toMm())
              : legend.topLeft();
          const qreal x = (p.x() < legend.center().x())
              ? (legend.left() - 0.4)
              : (legend.right() + 0.4);
          const qreal y = (p.y() < legend.center().y())
              ? (legend.top() - 0.4)
              : (legend.bottom() + 0.4);
          fpt->getCircles().append(std::make_shared<Circle>(
              Uuid::createRandom(), Layer::topLegend(), UnsignedLength(0), true,
              false, Point::fromMm(x, y), PositiveLength(400000)));
        }
        pkg.getFootprints().append(fpt);
        return McpToolResult::json(saveNewElement(session, pkg, libDir));
      },
  });

  server.addTool(McpTool{
      "component_create",
      "Create a component from one symbol, or from several gates (e.g. the "
      "two halves and the supply of a dual op-amp). One signal is created "
      "for each symbol pin, named like the pin; a pin name used in several "
      "gates gets the gate suffix ('OUT_A'). Signals are passive and "
      "optional unless set in 'signals'; the ERC reports unconnected "
      "required signals. Returns the result of the component check.",
      objectSchema(
          {{"library", stringProp("Name of the local library")},
           {"name", stringProp("Component name, e.g. 'LM317'")},
           {"description", stringProp("Optional description")},
           {"categories",
            arrayProp(stringProp("Name or UUID"), "Component categories")},
           {"symbol", stringProp("Name or UUID of the symbol of one gate")},
           {"gates",
            arrayProp(
                objectSchema(
                    {{"symbol", stringProp("Name or UUID of the symbol")},
                     {"suffix", stringProp("Gate suffix, e.g. 'A'")},
                     {"position",
                      pointProp("Position of the gate when placing the part, "
                                "default [0, 0]")}},
                    {"symbol", "suffix"}),
                "Gates, instead of 'symbol'")},
           {"signals",
            QJsonObject{
                {"type", "object"},
                {"additionalProperties",
                 objectSchema(
                     {{"role",
                       enumProp({"passive", "power", "input", "output", "inout",
                                 "opendrain"},
                                "Electrical role, default passive")},
                      {"required",
                       boolProp("Must be connected, default false")}})},
                {"description",
                 "Properties per signal name, e.g. {\"VCC\": {\"role\": "
                 "\"power\", \"required\": true}}"},
            }},
           {"prefix", stringProp("Designator prefix, e.g. 'U'")},
           {"defaultValue",
            stringProp("Default value, e.g. '{{MPN or DEVICE}}'")}},
          {"library", "name", "prefix"}),
      false,
      [&session](const QJsonObject& args) {
        Workspace& ws = session.getWorkspace();
        const WorkspaceLibraryDb& db = ws.getLibraryDb();
        const FilePath libDir =
            findLocalLibrary(ws, requiredString(args, "library"));
        QJsonArray gates = args.value("gates").toArray();
        if (args.contains("symbol") == args.contains("gates")) {
          throw RuntimeError(__FILE__, __LINE__,
                             "Give either 'symbol' or 'gates'.");
        } else if (args.contains("symbol")) {
          gates = QJsonArray{
              QJsonObject{{"symbol", args.value("symbol")}, {"suffix", ""}}};
        }
        struct Gate {
          Uuid symbol;
          std::shared_ptr<Symbol> lib;
          QString suffix;
          Point position;
        };
        QList<Gate> gateList;
        QHash<QString, int> pinCount;
        for (const QJsonValue& value : std::as_const(gates)) {
          const QJsonObject obj = value.toObject();
          const auto found =
              findLibraryElement<Symbol>(db, requiredString(obj, "symbol"));
          std::shared_ptr<Symbol> sym = openElement<Symbol>(found.second);
          for (const SymbolPin& pin : sym->getPins()) {
            ++pinCount[*pin.getName()];
          }
          gateList.append(Gate{found.first, sym, obj.value("suffix").toString(),
                               obj.contains("position")
                                   ? pointMm(obj.value("position"), "position")
                                   : Point()});
        }
        const QHash<QString, const SignalRole*> roles = {
            {"passive", &SignalRole::passive()},
            {"power", &SignalRole::power()},
            {"input", &SignalRole::input()},
            {"output", &SignalRole::output()},
            {"inout", &SignalRole::inout()},
            {"opendrain", &SignalRole::opendrain()},
        };
        const QJsonObject signalProps = args.value("signals").toObject();
        Component cmp(Uuid::createRandom(), initialVersion(),
                      ws.getSettings().userName.get(),
                      QDateTime::currentDateTime(), elementName(args),
                      args.value("description").toString(), QString());
        cmp.setCategories(
            findCategories<ComponentCategory>(db, args.value("categories")));
        cmp.setPrefixes(NormDependentPrefixMap(ComponentPrefix(
            cleanComponentPrefix(requiredString(args, "prefix")))));
        cmp.setDefaultValue(args.value("defaultValue").toString());
        auto variant = std::make_shared<ComponentSymbolVariant>(
            Uuid::createRandom(), "", ElementName("default"), "");
        for (const Gate& g : gateList) {
          auto gate = std::make_shared<ComponentSymbolVariantItem>(
              Uuid::createRandom(), g.symbol, g.position, Angle(), true,
              ComponentSymbolVariantItemSuffix(
                  cleanComponentSymbolVariantItemSuffix(g.suffix)));
          for (const SymbolPin& pin : g.lib->getPins()) {
            const QString name = (pinCount.value(*pin.getName()) > 1)
                ? (*pin.getName() % "_" % g.suffix)
                : *pin.getName();
            if (cmp.getSignals().contains(name)) {
              throw RuntimeError(
                  __FILE__, __LINE__,
                  QString("Several pins would get the signal name '%1', give "
                          "the gates different suffixes.")
                      .arg(name));
            }
            const QJsonObject props = signalProps.value(name).toObject();
            const SignalRole* role =
                roles.value(props.value("role").toString("passive"));
            auto sig = std::make_shared<ComponentSignal>(
                Uuid::createRandom(), CircuitIdentifier(name), *role, QString(),
                props.value("required").toBool(false), false, false);
            cmp.getSignals().append(sig);
            gate->getPinSignalMap().append(
                std::make_shared<ComponentPinSignalMapItem>(
                    pin.getUuid(), sig->getUuid(),
                    CmpSigPinDisplayType::componentSignal()));
          }
          variant->getSymbolItems().append(gate);
        }
        for (auto it = signalProps.begin(); it != signalProps.end(); ++it) {
          if (!cmp.getSignals().contains(it.key())) {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("There is no signal '%1' for 'signals'.")
                                   .arg(it.key()));
          }
        }
        cmp.getSymbolVariants().append(variant);
        return McpToolResult::json(saveNewElement(session, cmp, libDir));
      },
  });

  server.addTool(McpTool{
      "device_create",
      "Create a device which maps the signals of a component to the pads of "
      "a package. Returns the result of the device check.",
      objectSchema(
          {{"library", stringProp("Name of the local library")},
           {"name", stringProp("Device name, e.g. 'LM317 SOT-223'")},
           {"description", stringProp("Optional description")},
           {"categories",
            arrayProp(stringProp("Name or UUID"), "Component categories")},
           {"component", stringProp("Name or UUID of the component")},
           {"package", stringProp("Name or UUID of the package")},
           {"padSignalMap",
            QJsonObject{
                {"type", "object"},
                {"additionalProperties", QJsonObject{{"type", "string"}}},
                {"description",
                 "Signal name for each pad name, e.g. {\"1\": \"ADJ\"}"},
            }},
           {"parts",
            arrayProp(
                objectSchema({{"mpn", stringProp("Part number")},
                              {"manufacturer", stringProp("Manufacturer")}},
                             {"mpn"}),
                "Orderable parts")}},
          {"library", "name", "component", "package", "padSignalMap"}),
      false,
      [&session](const QJsonObject& args) {
        Workspace& ws = session.getWorkspace();
        const WorkspaceLibraryDb& db = ws.getLibraryDb();
        const FilePath libDir =
            findLocalLibrary(ws, requiredString(args, "library"));
        const auto cmpFound = findLibraryElement<Component>(
            db, requiredString(args, "component"));
        const auto pkgFound =
            findLibraryElement<Package>(db, requiredString(args, "package"));
        std::unique_ptr<Component> cmp =
            openElement<Component>(cmpFound.second);
        std::unique_ptr<Package> pkg = openElement<Package>(pkgFound.second);
        Device dev(Uuid::createRandom(), initialVersion(),
                   ws.getSettings().userName.get(),
                   QDateTime::currentDateTime(), elementName(args),
                   args.value("description").toString(), QString(),
                   cmpFound.first, pkgFound.first);
        dev.setCategories(
            findCategories<ComponentCategory>(db, args.value("categories")));
        const QJsonObject map = args.value("padSignalMap").toObject();
        for (auto it = map.begin(); it != map.end(); ++it) {
          if (!pkg->getPads().contains(it.key())) {
            throw RuntimeError(
                __FILE__, __LINE__,
                QString("The package has no pad named '%1'.").arg(it.key()));
          }
        }
        for (const PackagePad& pad : pkg->getPads()) {
          std::optional<Uuid> signal;
          const QString sigName = map.value(*pad.getName()).toString();
          if (!sigName.isEmpty()) {
            const auto sig = cmp->getSignals().find(sigName);
            if (!sig) {
              throw RuntimeError(
                  __FILE__, __LINE__,
                  QString("The component has no signal named '%1'.")
                      .arg(sigName));
            }
            signal = sig->getUuid();
          }
          dev.getPadSignalMap().append(
              std::make_shared<DevicePadSignalMapItem>(pad.getUuid(), signal));
        }
        const QJsonArray partValues = args.value("parts").toArray();
        for (const QJsonValue& value : partValues) {
          const QJsonObject part = value.toObject();
          dev.getParts().append(std::make_shared<Part>(
              SimpleString(cleanSimpleString(requiredString(part, "mpn"))),
              SimpleString(
                  cleanSimpleString(part.value("manufacturer").toString())),
              AttributeList()));
        }
        return McpToolResult::json(saveNewElement(session, dev, libDir));
      },
  });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
