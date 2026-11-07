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
#include "mcpserver.h"

#include <librepcb/core/exceptions.h>

#include <QtCore>

#include <algorithm>
#include <cmath>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Argument Validation
 ******************************************************************************/

static bool hasType(const QJsonValue& value, const QString& type) noexcept {
  if (type == "object") return value.isObject();
  if (type == "array") return value.isArray();
  if (type == "string") return value.isString();
  if (type == "boolean") return value.isBool();
  if (type == "null") return value.isNull();
  if (type == "number") return value.isDouble();
  if (type == "integer") {
    return value.isDouble() &&
        (value.toDouble() == std::floor(value.toDouble()));
  }
  return true;
}

/**
 * @brief Check a value against the subset of JSON schema used by the tools
 *
 * @throws ::librepcb::RuntimeError with a message for the MCP client.
 */
static void validate(const QJsonValue& value, const QJsonObject& schema,
                     const QString& path) {
  const QJsonValue typeValue = schema.value("type");
  QStringList types;
  const QJsonArray typeValues =
      typeValue.isArray() ? typeValue.toArray() : QJsonArray{typeValue};
  for (const QJsonValue& t : typeValues) {
    if (t.isString()) types.append(t.toString());
  }
  if ((!types.isEmpty()) &&
      std::none_of(types.begin(), types.end(),
                   [&](const QString& t) { return hasType(value, t); })) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Argument '%1' must be of type %2.")
                           .arg(path, types.join(" or ")));
  }
  const QJsonArray enumValues = schema.value("enum").toArray();
  if ((!enumValues.isEmpty()) && (!enumValues.contains(value))) {
    QStringList names;
    for (const QJsonValue& v : enumValues) {
      names.append(v.toVariant().toString());
    }
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Argument '%1' must be one of: %2.")
                           .arg(path, names.join(", ")));
  }
  if (value.isDouble()) {
    const QJsonValue minimum = schema.value("minimum");
    const QJsonValue maximum = schema.value("maximum");
    const double number = value.toDouble();
    if ((minimum.isDouble() && (number < minimum.toDouble())) ||
        (maximum.isDouble() && (number > maximum.toDouble()))) {
      QString msg;
      if (minimum.isDouble() && maximum.isDouble()) {
        msg = QString("Argument '%1' must be between %2 and %3.")
                  .arg(path)
                  .arg(minimum.toDouble())
                  .arg(maximum.toDouble());
      } else if (minimum.isDouble()) {
        msg = QString("Argument '%1' must be at least %2.")
                  .arg(path)
                  .arg(minimum.toDouble());
      } else {
        msg = QString("Argument '%1' must be at most %2.")
                  .arg(path)
                  .arg(maximum.toDouble());
      }
      throw RuntimeError(__FILE__, __LINE__, msg);
    }
  }
  if (value.isObject()) {
    const QJsonObject obj = value.toObject();
    const QJsonObject properties = schema.value("properties").toObject();
    const QJsonValue additional = schema.value("additionalProperties");
    for (auto it = obj.begin(); it != obj.end(); ++it) {
      const QString subPath =
          path.isEmpty() ? it.key() : (path % "." % it.key());
      if (properties.contains(it.key())) {
        validate(it.value(), properties.value(it.key()).toObject(), subPath);
      } else if (additional.isObject()) {
        validate(it.value(), additional.toObject(), subPath);
      } else if (additional.toBool(false)) {
        continue;
      } else if (path.isEmpty() && properties.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__, "The tool takes no arguments.");
      } else {
        throw RuntimeError(__FILE__, __LINE__,
                           QString("Unknown argument '%1'. Valid: %2.")
                               .arg(subPath, properties.keys().join(", ")));
      }
    }
    const QJsonArray requiredKeys = schema.value("required").toArray();
    for (const QJsonValue& key : requiredKeys) {
      if (!obj.contains(key.toString())) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("Argument '%1' is required.")
                .arg(path.isEmpty() ? key.toString()
                                    : (path % "." % key.toString())));
      }
    }
  } else if (value.isArray()) {
    const QJsonArray array = value.toArray();
    const QJsonValue minItems = schema.value("minItems");
    const QJsonValue maxItems = schema.value("maxItems");
    if ((minItems.isDouble() && (array.count() < minItems.toInt())) ||
        (maxItems.isDouble() && (array.count() > maxItems.toInt()))) {
      throw RuntimeError(
          __FILE__, __LINE__,
          (minItems == maxItems)
              ? QString("Argument '%1' must have %2 items.")
                    .arg(path)
                    .arg(minItems.toInt())
              : QString("Argument '%1' has the wrong number of items.")
                    .arg(path));
    }
    const QJsonObject items = schema.value("items").toObject();
    for (int i = 0; i < array.count(); ++i) {
      validate(array.at(i), items, QString("%1[%2]").arg(path).arg(i));
    }
  }
}

/*******************************************************************************
 *  Struct McpToolResult
 ******************************************************************************/

McpToolResult McpToolResult::text(const QString& text) noexcept {
  McpToolResult result;
  result.content.append(QJsonObject{{"type", "text"}, {"text", text}});
  return result;
}

McpToolResult McpToolResult::json(const QJsonObject& obj) noexcept {
  McpToolResult result = text(
      QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
  result.structuredContent = obj;
  return result;
}

McpToolResult McpToolResult::image(const QByteArray& png) noexcept {
  McpToolResult result;
  result.content.append(QJsonObject{
      {"type", "image"},
      {"data", QString::fromLatin1(png.toBase64())},
      {"mimeType", "image/png"},
  });
  return result;
}

McpToolResult McpToolResult::error(const QString& msg) noexcept {
  McpToolResult result = text(msg);
  result.isError = true;
  return result;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

McpServer::McpServer(const QString& name, const QString& version) noexcept
  : mName(name), mVersion(version) {
}

McpServer::~McpServer() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void McpServer::addTool(const McpTool& tool) noexcept {
  Q_ASSERT(tool.handler);
  mTools.append(tool);
}

std::optional<QJsonObject> McpServer::handleMessage(
    const QJsonObject& msg) noexcept {
  const QJsonValue id = msg.value("id");
  if ((!msg.contains("method")) || (!msg.contains("id"))) {
    // Notifications and responses don't get a response.
    return std::nullopt;
  }
  if ((msg.value("jsonrpc").toString() != "2.0") ||
      (!msg.value("method").isString())) {
    return makeError(id, -32600, "Invalid Request");
  }

  const QString method = msg.value("method").toString();
  const QJsonObject params = msg.value("params").toObject();
  if (method == "initialize") {
    return makeResult(id, initialize(params));
  } else if (method == "ping") {
    return makeResult(id, QJsonObject());
  } else if (method == "tools/list") {
    return makeResult(id, listTools());
  } else if (method == "tools/call") {
    const QString name = params.value("name").toString();
    for (const McpTool& tool : std::as_const(mTools)) {
      if (tool.name == name) {
        return makeResult(id, callTool(params));
      }
    }
    return makeError(id, -32602, "Unknown tool: " % name);
  } else {
    return makeError(id, -32601, "Method not found: " % method);
  }
}

QByteArray McpServer::handleLine(const QByteArray& line) noexcept {
  if (line.trimmed().isEmpty()) {
    return QByteArray();
  }
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
  std::optional<QJsonObject> response;
  if (parseError.error != QJsonParseError::NoError) {
    response = makeError(QJsonValue::Null, -32700, "Parse error");
  } else if (!doc.isObject()) {
    response = makeError(QJsonValue::Null, -32600, "Invalid Request");
  } else {
    response = handleMessage(doc.object());
  }
  if (!response) {
    return QByteArray();
  }
  return QJsonDocument(*response).toJson(QJsonDocument::Compact) % "\n";
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QStringList McpServer::getSupportedProtocolVersions() noexcept {
  return {"2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05"};
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QJsonObject McpServer::initialize(const QJsonObject& params) const noexcept {
  const QStringList versions = getSupportedProtocolVersions();
  QString version = params.value("protocolVersion").toString();
  if (!versions.contains(version)) {
    version = versions.first();
  }
  return QJsonObject{
      {"protocolVersion", version},
      {"capabilities", QJsonObject{{"tools", QJsonObject()}}},
      {"serverInfo", QJsonObject{{"name", mName}, {"version", mVersion}}},
      {"instructions",
       "Inspect and edit LibrePCB projects. Lengths are in millimeters, "
       "angles in degrees, board coordinates have Y up. Parts are addressed "
       "by their designator (e.g. 'U1'), pins as '<designator>.<signal>' "
       "(e.g. 'U1.VCC'), pads as '<designator>.<pad>' and other board items "
       "by IDs like 'trace:<uuid>' (see board_list_items). Copper layers are "
       "'top', 'bottom' and 'inner1' ... if the board has inner layers."},
  };
}

QJsonObject McpServer::listTools() const noexcept {
  QJsonArray tools;
  for (const McpTool& tool : mTools) {
    QJsonObject schema = tool.inputSchema;
    if (schema.isEmpty()) {
      schema = QJsonObject{{"type", "object"}};
    }
    if (!schema.contains("additionalProperties")) {
      schema.insert("additionalProperties", false);
    }
    tools.append(QJsonObject{
        {"name", tool.name},
        {"description", tool.description},
        {"inputSchema", schema},
        {"annotations", QJsonObject{{"readOnlyHint", tool.readOnly}}},
    });
  }
  return QJsonObject{{"tools", tools}};
}

QJsonObject McpServer::callTool(const QJsonObject& params) const noexcept {
  const QString name = params.value("name").toString();
  const QJsonObject args = params.value("arguments").toObject();
  McpToolResult result;
  for (const McpTool& tool : mTools) {
    if (tool.name == name) {
      try {
        validate(args,
                 tool.inputSchema.isEmpty() ? QJsonObject{{"type", "object"}}
                                            : tool.inputSchema,
                 QString());
        result = tool.handler(args);
      } catch (const Exception& e) {
        result = McpToolResult::error(e.getMsg());
      } catch (const std::exception& e) {
        result = McpToolResult::error(QString::fromUtf8(e.what()));
      }
      break;
    }
  }
  QJsonObject obj{{"content", result.content}, {"isError", result.isError}};
  if (!result.structuredContent.isEmpty()) {
    obj.insert("structuredContent", result.structuredContent);
  }
  return obj;
}

QJsonObject McpServer::makeResult(const QJsonValue& id,
                                  const QJsonObject& result) noexcept {
  return QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}

QJsonObject McpServer::makeError(const QJsonValue& id, int code,
                                 const QString& message) noexcept {
  return QJsonObject{
      {"jsonrpc", "2.0"},
      {"id", id},
      {"error", QJsonObject{{"code", code}, {"message", message}}},
  };
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
