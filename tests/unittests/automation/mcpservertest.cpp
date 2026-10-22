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

#include <gtest/gtest.h>
#include <librepcb/automation/mcpserver.h>
#include <librepcb/core/exceptions.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class McpServerTest : public ::testing::Test {
protected:
  McpServerTest() : mServer("librepcb", "1.2.3") {
    mServer.addTool(McpTool{
        "echo",
        "Echo the argument.",
        QJsonObject{
            {"type", "object"},
            {"properties",
             QJsonObject{
                 {"text", QJsonObject{{"type", "string"}}},
                 {"count",
                  QJsonObject{
                      {"type", "integer"}, {"minimum", 1}, {"maximum", 10}}},
                 {"items",
                  QJsonObject{
                      {"type", "array"},
                      {"items",
                       QJsonObject{
                           {"type", "object"},
                           {"properties",
                            QJsonObject{
                                {"x", QJsonObject{{"type", "number"}}}}},
                       }},
                  }},
             }},
            {"required", QJsonArray{"text"}},
        },
        true,
        [](const QJsonObject& args) {
          return McpToolResult::text(args.value("text").toString());
        },
    });
    mServer.addTool(McpTool{
        "fail",
        "Always throws.",
        QJsonObject(),
        false,
        [](const QJsonObject&) -> McpToolResult {
          throw RuntimeError(__FILE__, __LINE__, "Boom");
        },
    });
  }

  QJsonObject request(const QString& method,
                      const QJsonObject& params = QJsonObject()) {
    const std::optional<QJsonObject> response =
        mServer.handleMessage(QJsonObject{{"jsonrpc", "2.0"},
                                          {"id", 42},
                                          {"method", method},
                                          {"params", params}});
    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(42, response.value_or(QJsonObject()).value("id").toInt());
    return response.value_or(QJsonObject());
  }

  McpServer mServer;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(McpServerTest, testInitializeWithSupportedVersion) {
  const QJsonObject result =
      request("initialize", {{"protocolVersion", "2025-06-18"}})
          .value("result")
          .toObject();
  EXPECT_EQ("2025-06-18", result.value("protocolVersion").toString());
  EXPECT_EQ("librepcb",
            result.value("serverInfo").toObject().value("name").toString());
  EXPECT_EQ("1.2.3",
            result.value("serverInfo").toObject().value("version").toString());
  EXPECT_TRUE(result.value("capabilities").toObject().contains("tools"));
}

TEST_F(McpServerTest, testInitializeWithUnsupportedVersion) {
  const QJsonObject result =
      request("initialize", {{"protocolVersion", "1999-01-01"}})
          .value("result")
          .toObject();
  EXPECT_EQ(McpServer::getSupportedProtocolVersions().first(),
            result.value("protocolVersion").toString());
}

TEST_F(McpServerTest, testPing) {
  const QJsonObject response = request("ping");
  EXPECT_TRUE(response.value("result").isObject());
  EXPECT_FALSE(response.contains("error"));
}

TEST_F(McpServerTest, testListTools) {
  const QJsonArray tools =
      request("tools/list").value("result").toObject().value("tools").toArray();
  ASSERT_EQ(2, tools.count());
  const QJsonObject echo = tools.at(0).toObject();
  EXPECT_EQ("echo", echo.value("name").toString());
  EXPECT_TRUE(
      echo.value("annotations").toObject().value("readOnlyHint").toBool(false));
  const QJsonObject fail = tools.at(1).toObject();
  EXPECT_EQ("object",
            fail.value("inputSchema").toObject().value("type").toString());
  EXPECT_FALSE(
      fail.value("annotations").toObject().value("readOnlyHint").toBool(true));
}

TEST_F(McpServerTest, testCallTool) {
  const QJsonObject result =
      request("tools/call",
              {{"name", "echo"}, {"arguments", QJsonObject{{"text", "hi"}}}})
          .value("result")
          .toObject();
  EXPECT_FALSE(result.value("isError").toBool(true));
  const QJsonArray content = result.value("content").toArray();
  ASSERT_EQ(1, content.count());
  EXPECT_EQ("text", content.at(0).toObject().value("type").toString());
  EXPECT_EQ("hi", content.at(0).toObject().value("text").toString());
}

TEST_F(McpServerTest, testCallToolThrowing) {
  const QJsonObject result =
      request("tools/call", {{"name", "fail"}}).value("result").toObject();
  EXPECT_TRUE(result.value("isError").toBool(false));
  EXPECT_EQ("Boom",
            result.value("content")
                .toArray()
                .at(0)
                .toObject()
                .value("text")
                .toString());
}

TEST_F(McpServerTest, testCallToolWithUnknownArgument) {
  const QJsonObject result =
      request("tools/call",
              {{"name", "echo"},
               {"arguments", QJsonObject{{"text", "hi"}, {"txt", "hi"}}}})
          .value("result")
          .toObject();
  EXPECT_TRUE(result.value("isError").toBool(false));
  const QString msg = result.value("content")
                          .toArray()
                          .at(0)
                          .toObject()
                          .value("text")
                          .toString();
  EXPECT_TRUE(msg.contains("Unknown argument 'txt'")) << msg.toStdString();
  EXPECT_TRUE(msg.contains("Valid: count, items, text.")) << msg.toStdString();
}

TEST_F(McpServerTest, testCallToolWithUnknownNestedArgument) {
  const QJsonObject result =
      request("tools/call",
              {{"name", "echo"},
               {"arguments",
                QJsonObject{{"text", "hi"},
                            {"items",
                             QJsonArray{QJsonObject{{"x", 1}},
                                        QJsonObject{{"y", 2}}}}}}})
          .value("result")
          .toObject();
  EXPECT_TRUE(result.value("isError").toBool(false));
  const QString msg = result.value("content")
                          .toArray()
                          .at(0)
                          .toObject()
                          .value("text")
                          .toString();
  EXPECT_TRUE(msg.contains("Unknown argument 'items[1].y'"))
      << msg.toStdString();
}

TEST_F(McpServerTest, testCallToolWithWrongArgumentType) {
  const QJsonObject result =
      request("tools/call",
              {{"name", "echo"}, {"arguments", QJsonObject{{"text", 5}}}})
          .value("result")
          .toObject();
  EXPECT_TRUE(result.value("isError").toBool(false));
  const QString msg = result.value("content")
                          .toArray()
                          .at(0)
                          .toObject()
                          .value("text")
                          .toString();
  EXPECT_TRUE(msg.contains("'text' must be of type string"))
      << msg.toStdString();
}

TEST_F(McpServerTest, testCallToolWithNumberOutOfRange) {
  for (int count : {0, 11}) {
    const QJsonObject result =
        request("tools/call",
                {{"name", "echo"},
                 {"arguments", QJsonObject{{"text", "hi"}, {"count", count}}}})
            .value("result")
            .toObject();
    EXPECT_TRUE(result.value("isError").toBool(false));
    const QString msg = result.value("content")
                            .toArray()
                            .at(0)
                            .toObject()
                            .value("text")
                            .toString();
    EXPECT_TRUE(msg.contains("Argument 'count' must be between 1 and 10."))
        << msg.toStdString();
  }
}

TEST_F(McpServerTest, testCallToolWithoutRequiredArgument) {
  const QJsonObject result =
      request("tools/call", {{"name", "echo"}, {"arguments", QJsonObject{}}})
          .value("result")
          .toObject();
  EXPECT_TRUE(result.value("isError").toBool(false));
  const QString msg = result.value("content")
                          .toArray()
                          .at(0)
                          .toObject()
                          .value("text")
                          .toString();
  EXPECT_TRUE(msg.contains("Argument 'text' is required")) << msg.toStdString();
}

TEST_F(McpServerTest, testCallToolWithoutArgumentsRejectsArguments) {
  const QJsonObject result =
      request("tools/call",
              {{"name", "fail"}, {"arguments", QJsonObject{{"force", true}}}})
          .value("result")
          .toObject();
  EXPECT_TRUE(result.value("isError").toBool(false));
  const QString msg = result.value("content")
                          .toArray()
                          .at(0)
                          .toObject()
                          .value("text")
                          .toString();
  EXPECT_TRUE(msg.contains("The tool takes no arguments."))
      << msg.toStdString();
}

TEST_F(McpServerTest, testCallUnknownTool) {
  const QJsonObject error =
      request("tools/call", {{"name", "nope"}}).value("error").toObject();
  EXPECT_EQ(-32602, error.value("code").toInt());
}

TEST_F(McpServerTest, testUnknownMethod) {
  const QJsonObject error = request("foo/bar").value("error").toObject();
  EXPECT_EQ(-32601, error.value("code").toInt());
}

TEST_F(McpServerTest, testNotificationHasNoResponse) {
  EXPECT_FALSE(mServer
                   .handleMessage(QJsonObject{
                       {"jsonrpc", "2.0"},
                       {"method", "notifications/initialized"},
                   })
                   .has_value());
}

TEST_F(McpServerTest, testHandleLine) {
  const QByteArray response =
      mServer.handleLine(R"({"jsonrpc":"2.0","id":"a","method":"ping"})");
  EXPECT_EQ(R"({"id":"a","jsonrpc":"2.0","result":{}})"
            "\n",
            response.toStdString());
}

TEST_F(McpServerTest, testHandleLineParseError) {
  const QJsonObject response =
      QJsonDocument::fromJson(mServer.handleLine("{nope")).object();
  EXPECT_EQ(-32700, response.value("error").toObject().value("code").toInt());
  EXPECT_TRUE(response.value("id").isNull());
}

TEST_F(McpServerTest, testHandleEmptyLine) {
  EXPECT_TRUE(mServer.handleLine("  ").isEmpty());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace automation
}  // namespace librepcb
