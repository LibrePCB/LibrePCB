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
#include <librepcb/automation/mcpstdiotransport.h>

#include <QtCore>

#include <sstream>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class McpStdioTransportTest : public ::testing::Test {
protected:
  McpStdioTransportTest() : mServer("librepcb", "1.2.3") {
    mServer.addTool(McpTool{
        "wait",
        "Runs a nested event loop.",
        QJsonObject(),
        true,
        [](const QJsonObject&) {
          QEventLoop loop;
          QTimer::singleShot(50, &loop, &QEventLoop::quit);
          loop.exec();
          return McpToolResult::text("waited");
        },
    });
    mServer.addTool(McpTool{
        "now",
        "Answers immediately.",
        QJsonObject(),
        true,
        [](const QJsonObject&) { return McpToolResult::text("now"); },
    });
  }

  static std::string call(int id, const QString& tool) {
    const QJsonObject msg{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "tools/call"},
        {"params", QJsonObject{{"name", tool}}},
    };
    return QJsonDocument(msg).toJson(QJsonDocument::Compact).toStdString() +
        "\n";
  }

  QList<QJsonObject> run(const std::string& input) {
    std::istringstream in(input);
    std::ostringstream out;
    McpStdioTransport transport(mServer, in, out);
    QEventLoop loop;
    QObject::connect(&transport, &McpStdioTransport::finished, &loop,
                     &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    transport.start();
    loop.exec();
    QList<QJsonObject> responses;
    for (const QByteArray& line :
         QByteArray::fromStdString(out.str()).split('\n')) {
      if (!line.isEmpty()) {
        responses.append(QJsonDocument::fromJson(line).object());
      }
    }
    return responses;
  }

  McpServer mServer;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(McpStdioTransportTest, testEmptyInputFinishes) {
  EXPECT_TRUE(run("").isEmpty());
}

TEST_F(McpStdioTransportTest, testResponsesKeepOrderDuringNestedEventLoop) {
  const QList<QJsonObject> responses =
      run(call(1, "wait") + call(2, "now") +
          "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}\n");
  ASSERT_EQ(2, responses.count());
  EXPECT_EQ(1, responses.at(0).value("id").toInt());
  EXPECT_EQ(2, responses.at(1).value("id").toInt());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace automation
}  // namespace librepcb
