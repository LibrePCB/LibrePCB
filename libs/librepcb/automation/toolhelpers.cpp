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
#include "toolhelpers.h"

#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>
#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/transform.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>
#include <QtGui>

#include <cmath>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Argument Parsing
 ******************************************************************************/

QString requiredString(const QJsonObject& args, const QString& key) {
  const QString value = args.value(key).toString().trimmed();
  if (value.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Argument '%1' is required.").arg(key));
  }
  return value;
}

Length lengthMm(const QJsonValue& value, const QString& key) {
  if (!value.isDouble()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Argument '%1' must be a number (millimeters).").arg(key));
  }
  return Length::fromMm(value.toDouble());  // can throw
}

Point pointMm(const QJsonValue& value, const QString& key) {
  const QJsonArray array = value.toArray();
  if ((array.count() != 2) || (!array.at(0).isDouble()) ||
      (!array.at(1).isDouble())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Argument '%1' must be an array [x, y] in millimeters.")
            .arg(key));
  }
  return Point::fromMm(array.at(0).toDouble(),
                       array.at(1).toDouble());  // can throw
}

Angle angleDeg(const QJsonValue& value, const QString& key) {
  if (value.isUndefined() || value.isNull()) {
    return Angle::deg0();
  }
  if (!value.isDouble()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Argument '%1' must be a number (degrees).").arg(key));
  }
  return Angle::fromDeg(value.toDouble());  // can throw
}

Path closedPathMm(const QJsonValue& value, const QString& key) {
  const QJsonArray array = value.toArray();
  if (array.count() < 3) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Argument '%1' needs at least 3 vertices.").arg(key));
  }
  Path path;
  for (const QJsonValue& vertex : array) {
    const QJsonArray v = vertex.toArray();
    path.addVertex(pointMm(QJsonArray{v.at(0), v.at(1)}, key),
                   angleDeg(v.at(2), key));  // can throw
  }
  path.close();
  return path;
}

QJsonArray verticesToJson(const Path& path) noexcept {
  QJsonArray array;
  const QVector<Vertex>& vertices = path.getVertices();
  const int count = (path.isClosed() && (vertices.count() > 1))
      ? (vertices.count() - 1)
      : vertices.count();
  for (int i = 0; i < count; ++i) {
    QJsonArray vertex = toJson(vertices.at(i).getPos());
    if (vertices.at(i).getAngle() != Angle::deg0()) {
      vertex.append(vertices.at(i).getAngle().toDeg());
    }
    array.append(vertex);
  }
  return array;
}

QStringList zoneRuleNames() noexcept {
  return {"noCopper", "noPlanes", "noExposure", "noDevices"};
}

Zone::Rules zoneRules(const QJsonValue& value) {
  Zone::Rules rules;
  const QJsonArray ruleValues = value.toArray();
  for (const QJsonValue& rule : ruleValues) {
    if (rule == "noCopper") rules |= Zone::Rule::NoCopper;
    if (rule == "noPlanes") rules |= Zone::Rule::NoPlanes;
    if (rule == "noExposure") rules |= Zone::Rule::NoExposure;
    if (rule == "noDevices") rules |= Zone::Rule::NoDevices;
  }
  if (!rules) {
    throw RuntimeError(__FILE__, __LINE__, "No rules given.");
  }
  return rules;
}

QJsonArray zoneRulesToJson(Zone::Rules rules) noexcept {
  QJsonArray array;
  if (rules.testFlag(Zone::Rule::NoCopper)) array.append("noCopper");
  if (rules.testFlag(Zone::Rule::NoPlanes)) array.append("noPlanes");
  if (rules.testFlag(Zone::Rule::NoExposure)) array.append("noExposure");
  if (rules.testFlag(Zone::Rule::NoDevices)) array.append("noDevices");
  return array;
}

QJsonArray footprintZoneLayers(const Zone& zone, bool mirrored) noexcept {
  const bool top = zone.getLayers().testFlag(Zone::Layer::Top);
  const bool bottom = zone.getLayers().testFlag(Zone::Layer::Bottom);
  QJsonArray layers;
  if (mirrored ? bottom : top) layers.append("top");
  if (zone.getLayers().testFlag(Zone::Layer::Inner)) layers.append("inner");
  if (mirrored ? top : bottom) layers.append("bottom");
  return layers;
}

QJsonObject footprintGeometry(const Footprint& footprint,
                              const Transform& transform) noexcept {
  auto side = [](const Layer& layer) {
    return layer.isBottom() ? "bottom" : "top";
  };
  QJsonArray courtyard;
  for (const Polygon& polygon : footprint.getPolygons()) {
    const Layer& layer = transform.map(polygon.getLayer());
    if ((layer == Layer::topCourtyard()) || (layer == Layer::botCourtyard())) {
      courtyard.append(QJsonObject{
          {"side", side(layer)},
          {"vertices", verticesToJson(transform.map(polygon.getPath()))},
      });
    }
  }
  for (const Circle& circle : footprint.getCircles()) {
    const Layer& layer = transform.map(circle.getLayer());
    if ((layer == Layer::topCourtyard()) || (layer == Layer::botCourtyard())) {
      courtyard.append(QJsonObject{
          {"side", side(layer)},
          {"center", toJson(transform.map(circle.getCenter()))},
          {"diameter", circle.getDiameter()->toMm()},
      });
    }
  }
  QJsonArray keepouts;
  for (const Zone& zone : footprint.getZones()) {
    keepouts.append(QJsonObject{
        {"layers", footprintZoneLayers(zone, transform.getMirrored())},
        {"rules", zoneRulesToJson(zone.getRules())},
        {"vertices", verticesToJson(transform.map(zone.getOutline()))},
    });
  }
  QJsonArray holes;
  for (const Hole& hole : footprint.getHoles()) {
    holes.append(QJsonObject{
        {"position",
         toJson(transform.map(hole.getPath()->getVertices().first().getPos()))},
        {"diameter", hole.getDiameter()->toMm()},
    });
  }
  return QJsonObject{
      {"courtyard", courtyard},
      {"keepouts", keepouts},
      {"holes", holes},
  };
}

QJsonArray toJson(const Point& p) noexcept {
  return QJsonArray{p.getX().toMm(), p.getY().toMm()};
}

FilePath absoluteFilePath(const QString& path) {
  if (!QDir::isAbsolutePath(path)) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("The path '%1' must be absolute.").arg(path));
  }
  return FilePath(QFileInfo(path).absoluteFilePath());
}

static QString toString(RuleCheckMessage::Severity severity) noexcept {
  switch (severity) {
    case RuleCheckMessage::Severity::Hint:
      return "hint";
    case RuleCheckMessage::Severity::Warning:
      return "warning";
    default:
      return "error";
  }
}

QString messageId(const RuleCheckMessage& msg) noexcept {
  return QString::fromLatin1(
      QCryptographicHash::hash(msg.getApproval().toByteArray(),
                               QCryptographicHash::Sha1)
          .toHex()
          .left(12));
}

QJsonObject locationToJson(const Path& location) noexcept {
  const QRectF rect = location.toQPainterPathPx().boundingRect();
  auto mm = [](qreal px) {
    return std::round(Length::fromPx(px).toMm() * 1000) / 1000;
  };
  return QJsonObject{
      {"center", QJsonArray{mm(rect.center().x()), mm(-rect.center().y())}},
      {"size", QJsonArray{mm(rect.width()), mm(rect.height())}},
  };
}

QJsonObject toJson(const RuleCheckMessageList& messages,
                   const QSet<SExpression>& approvals) noexcept {
  const int maxLocations = 10;
  QJsonArray array;
  QSet<QString> descriptions;
  int approvedCount = 0;
  for (const auto& msg : messages) {
    const bool approved = approvals.contains(msg->getApproval());
    if (approved) {
      ++approvedCount;
    }
    QJsonObject obj{
        {"id", messageId(*msg)},
        {"severity", toString(msg->getSeverity())},
        {"message", msg->getMessage()},
        {"approved", approved},
    };
    if (!descriptions.contains(msg->getDescription())) {
      obj.insert("description", msg->getDescription());
      descriptions.insert(msg->getDescription());
    }
    QJsonArray locations;
    const auto shownLocations = msg->getLocations().mid(0, maxLocations);
    for (const Path& location : shownLocations) {
      locations.append(locationToJson(location));
    }
    if (!locations.isEmpty()) {
      obj.insert("locations", locations);
    }
    if (msg->getLocations().count() > maxLocations) {
      obj.insert("moreLocations", msg->getLocations().count() - maxLocations);
    }
    array.append(obj);
  }
  return QJsonObject{
      {"messages", array},
      {"approvedCount", approvedCount},
      {"nonApprovedCount", messages.count() - approvedCount},
  };
}

int approveMessages(const RuleCheckMessageList& messages,
                    const QSet<SExpression>& approvals, const QJsonValue& ids,
                    bool approved,
                    const std::function<void(const SExpression&, bool)>& set) {
  QHash<QString, const RuleCheckMessage*> byId;
  for (const auto& msg : messages) {
    byId.insert(messageId(*msg), msg.get());
  }
  QList<const RuleCheckMessage*> found;
  const QJsonArray idValues = ids.toArray();
  for (const QJsonValue& id : idValues) {
    const RuleCheckMessage* msg = byId.value(id.toString());
    if (!msg) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("No current message with ID '%1' found, run "
                                 "the check again to get the current IDs.")
                             .arg(id.toString()));
    }
    found.append(msg);
  }
  int changed = 0;
  for (const RuleCheckMessage* msg : found) {
    if (approvals.contains(msg->getApproval()) != approved) {
      ++changed;
    }
    set(msg->getApproval(), approved);
  }
  return changed;
}

QJsonObject objectSchema(const QJsonObject& properties,
                         const QStringList& required) noexcept {
  QJsonObject schema{{"type", "object"}, {"properties", properties}};
  if (!required.isEmpty()) {
    schema.insert("required", QJsonArray::fromStringList(required));
  }
  return schema;
}

QJsonObject stringProp(const QString& description) noexcept {
  return QJsonObject{{"type", "string"}, {"description", description}};
}

QJsonObject numberProp(const QString& description) noexcept {
  return QJsonObject{{"type", "number"}, {"description", description}};
}

QJsonObject boolProp(const QString& description) noexcept {
  return QJsonObject{{"type", "boolean"}, {"description", description}};
}

QJsonObject pointProp(const QString& description) noexcept {
  return QJsonObject{
      {"type", "array"},
      {"items", QJsonObject{{"type", "number"}}},
      {"minItems", 2},
      {"maxItems", 2},
      {"description", QString(description % " ([x, y] in mm)")},
  };
}

QJsonObject verticesProp(const QString& description) noexcept {
  return arrayProp(
      QJsonObject{
          {"type", "array"},
          {"items", QJsonObject{{"type", "number"}}},
          {"minItems", 2},
          {"maxItems", 3},
          {"description",
           "[x, y] in mm, or [x, y, angle] to draw an arc with this angle "
           "in degrees (positive = counter-clockwise) to the next vertex"},
      },
      description % " (closed automatically)");
}

QJsonObject enumProp(const QStringList& values,
                     const QString& description) noexcept {
  return QJsonObject{
      {"type", "string"},
      {"enum", QJsonArray::fromStringList(values)},
      {"description", description},
  };
}

QJsonObject arrayProp(const QJsonObject& items,
                      const QString& description) noexcept {
  return QJsonObject{
      {"type", "array"},
      {"items", items},
      {"description", description},
  };
}

/*******************************************************************************
 *  Lookups
 ******************************************************************************/

template <typename ElementType>
QList<Uuid> findLibraryElements(const WorkspaceLibraryDb& db,
                                const QString& keyword) {
  if constexpr (std::is_same_v<ElementType, Symbol> ||
                std::is_same_v<ElementType, Package> ||
                std::is_same_v<ElementType, Component> ||
                std::is_same_v<ElementType, Device>) {
    return db.find<ElementType>(keyword);  // can throw
  } else {
    // The database has no keyword search for these element types.
    QList<Uuid> result;
    for (const FilePath& fp : db.getAll<ElementType>()) {  // can throw
      Uuid uuid = Uuid::createRandom();
      QString name;
      if (db.getMetadata<ElementType>(fp, &uuid) &&
          db.getTranslations<ElementType>(fp, {}, &name) &&
          name.contains(keyword, Qt::CaseInsensitive) &&
          (!result.contains(uuid))) {
        result.append(uuid);
      }
    }
    return result;
  }
}

template <typename ElementType>
static bool hasElementNamed(const WorkspaceLibraryDb& db, const QString& name) {
  for (const Uuid& uuid :
       findLibraryElements<ElementType>(db, name)) {  // can throw
    QString elementName;
    db.getTranslations<ElementType>(db.getLatest<ElementType>(uuid), {},
                                    &elementName);  // can throw
    if (elementName.compare(name, Qt::CaseInsensitive) == 0) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Hint for a name which belongs to another element type
 */
static QString otherTypeHint(const WorkspaceLibraryDb& db, const QString& name,
                             const QString& searchedType) {
  const QList<
      std::pair<QString, bool (*)(const WorkspaceLibraryDb&, const QString&)>>
      types = {
          {Component::getLongElementName(), &hasElementNamed<Component>},
          {Device::getLongElementName(), &hasElementNamed<Device>},
          {Symbol::getLongElementName(), &hasElementNamed<Symbol>},
          {Package::getLongElementName(), &hasElementNamed<Package>},
          {ComponentCategory::getLongElementName(),
           &hasElementNamed<ComponentCategory>},
          {PackageCategory::getLongElementName(),
           &hasElementNamed<PackageCategory>},
      };
  for (const auto& [type, hasName] : types) {
    if ((type != searchedType) && hasName(db, name)) {  // can throw
      return QString(" Note: '%1' is a %2, not a %3.")
          .arg(name, type, searchedType);
    }
  }
  return QString();
}

template <typename ElementType>
std::pair<Uuid, FilePath> findLibraryElement(const WorkspaceLibraryDb& db,
                                             const QString& nameOrUuid) {
  if (const std::optional<Uuid> uuid = Uuid::tryFromString(nameOrUuid)) {
    const FilePath fp = db.getLatest<ElementType>(*uuid);  // can throw
    if (!fp.isValid()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("No %1 with UUID '%2' found in the workspace libraries.")
              .arg(ElementType::getLongElementName(), uuid->toStr()));
    }
    return std::make_pair(*uuid, fp);
  }

  QList<std::pair<Uuid, FilePath>> matches;
  QStringList others;
  for (const Uuid& uuid :
       findLibraryElements<ElementType>(db, nameOrUuid)) {  // can throw
    const FilePath fp = db.getLatest<ElementType>(uuid);  // can throw
    QString name;
    db.getTranslations<ElementType>(fp, {}, &name);  // can throw
    if (name.compare(nameOrUuid, Qt::CaseInsensitive) == 0) {
      matches.append(std::make_pair(uuid, fp));
    } else {
      others.append(QString("'%1'").arg(name));
    }
  }
  if (matches.count() == 1) {
    return matches.first();
  } else if (matches.count() > 1) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The %1 name '%2' is ambiguous, use its UUID instead.")
            .arg(ElementType::getLongElementName(), nameOrUuid));
  }
  others.sort();
  QString msg = QString("No %1 named '%2' found in the workspace libraries.")
                    .arg(ElementType::getLongElementName(), nameOrUuid);
  if (!others.isEmpty()) {
    msg += " Similar names: " % others.mid(0, 10).join(", ");
  }
  msg += otherTypeHint(db, nameOrUuid, ElementType::getLongElementName());
  throw RuntimeError(__FILE__, __LINE__, msg);
}

template QList<Uuid> findLibraryElements<ComponentCategory>(
    const WorkspaceLibraryDb& db, const QString& keyword);
template QList<Uuid> findLibraryElements<PackageCategory>(
    const WorkspaceLibraryDb& db, const QString& keyword);
template QList<Uuid> findLibraryElements<Symbol>(const WorkspaceLibraryDb& db,
                                                 const QString& keyword);
template QList<Uuid> findLibraryElements<Package>(const WorkspaceLibraryDb& db,
                                                  const QString& keyword);
template QList<Uuid> findLibraryElements<Component>(
    const WorkspaceLibraryDb& db, const QString& keyword);
template QList<Uuid> findLibraryElements<Device>(const WorkspaceLibraryDb& db,
                                                 const QString& keyword);
template QList<Uuid> findLibraryElements<Library>(const WorkspaceLibraryDb& db,
                                                  const QString& keyword);
template std::pair<Uuid, FilePath> findLibraryElement<ComponentCategory>(
    const WorkspaceLibraryDb& db, const QString& nameOrUuid);
template std::pair<Uuid, FilePath> findLibraryElement<PackageCategory>(
    const WorkspaceLibraryDb& db, const QString& nameOrUuid);
template std::pair<Uuid, FilePath> findLibraryElement<Symbol>(
    const WorkspaceLibraryDb& db, const QString& nameOrUuid);
template std::pair<Uuid, FilePath> findLibraryElement<Package>(
    const WorkspaceLibraryDb& db, const QString& nameOrUuid);
template std::pair<Uuid, FilePath> findLibraryElement<Component>(
    const WorkspaceLibraryDb& db, const QString& nameOrUuid);
template std::pair<Uuid, FilePath> findLibraryElement<Device>(
    const WorkspaceLibraryDb& db, const QString& nameOrUuid);
template std::pair<Uuid, FilePath> findLibraryElement<Library>(
    const WorkspaceLibraryDb& db, const QString& nameOrUuid);

QString partValue(const ComponentInstance& cmp) noexcept {
  std::shared_ptr<const Part> part;
  if (const auto option = cmp.getAssemblyOptions().value(0)) {
    part = option->getParts().value(0);
  }
  ProjectAttributeLookup lookup(cmp, nullptr, part);
  return AttributeSubstitutor::substitute(lookup("VALUE"), lookup);
}

ComponentInstance& findComponentInstance(Project& project,
                                         const QString& designator) {
  ComponentInstance* cmp =
      project.getCircuit().getComponentInstanceByName(designator);
  if (!cmp) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("No part named '%1' found.").arg(designator));
  }
  return *cmp;
}

void rebuildPlanes(Board& board) {
  const auto layers = board.getCopperLayers();
  BoardPlaneFragmentsBuilder builder;
  if (builder.start(board, &layers)) {
    BoardPlaneFragmentsBuilder::Result result = builder.waitForFinished();
    result.throwOnError();  // can throw
    result.applyToBoard();
  }
}

std::pair<QString, QString> splitPinRef(const QString& ref) {
  const int index = ref.indexOf('.');
  if ((index <= 0) || (index >= ref.length() - 1)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Invalid pin reference '%1', expected '<designator>.<pin>' "
                "(e.g. 'U1.VCC').")
            .arg(ref));
  }
  return std::make_pair(ref.left(index), ref.mid(index + 1));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
