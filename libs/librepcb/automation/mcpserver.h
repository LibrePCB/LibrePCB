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

#ifndef LIBREPCB_AUTOMATION_MCPSERVER_H
#define LIBREPCB_AUTOMATION_MCPSERVER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <functional>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Struct McpToolResult
 ******************************************************************************/

/**
 * @brief Result of an MCP tool call (the `CallToolResult` of the protocol)
 */
struct McpToolResult {
  QJsonArray content;
  QJsonObject structuredContent;
  bool isError = false;

  static McpToolResult text(const QString& text) noexcept;
  static McpToolResult json(const QJsonObject& obj) noexcept;
  static McpToolResult image(const QByteArray& png) noexcept;
  static McpToolResult error(const QString& msg) noexcept;
};

/*******************************************************************************
 *  Struct McpTool
 ******************************************************************************/

/**
 * @brief An MCP tool definition with its handler
 *
 * The handler may throw ::librepcb::Exception, which is reported to the client
 * as a tool error (not as a protocol error).
 */
struct McpTool {
  QString name;
  QString description;
  QJsonObject inputSchema;
  bool readOnly;
  std::function<McpToolResult(const QJsonObject& args)> handler;
};

/*******************************************************************************
 *  Class McpServer
 ******************************************************************************/

/**
 * @brief Model Context Protocol server (JSON-RPC 2.0), independent of the
 *        transport
 */
class McpServer final {
  Q_DECLARE_TR_FUNCTIONS(McpServer)

public:
  // Constructors / Destructor
  McpServer() = delete;
  McpServer(const McpServer& other) = delete;
  McpServer(const QString& name, const QString& version) noexcept;
  ~McpServer() noexcept;

  // General Methods
  void addTool(const McpTool& tool) noexcept;
  std::optional<QJsonObject> handleMessage(const QJsonObject& msg) noexcept;
  QByteArray handleLine(const QByteArray& line) noexcept;

  // Static Methods
  static QStringList getSupportedProtocolVersions() noexcept;

  // Operator Overloadings
  McpServer& operator=(const McpServer& rhs) = delete;

private:  // Methods
  QJsonObject initialize(const QJsonObject& params) const noexcept;
  QJsonObject listTools() const noexcept;
  QJsonObject callTool(const QJsonObject& params) const noexcept;
  static QJsonObject makeResult(const QJsonValue& id,
                                const QJsonObject& result) noexcept;
  static QJsonObject makeError(const QJsonValue& id, int code,
                               const QString& message) noexcept;

private:  // Data
  QString mName;
  QString mVersion;
  QVector<McpTool> mTools;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb

#endif
