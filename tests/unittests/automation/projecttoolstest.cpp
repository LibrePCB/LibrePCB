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
#include <librepcb/automation/designsession.h>
#include <librepcb/automation/mcpserver.h>
#include <librepcb/automation/projecttools.h>

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

class ProjectToolsTest : public ::testing::Test {
protected:
  ProjectToolsTest() : mServer("librepcb", "1.2.3") {
    addProjectTools(mServer, mSession);
  }

  QJsonObject call(const QString& tool,
                   const QJsonObject& args = QJsonObject()) {
    const std::optional<QJsonObject> response =
        mServer.handleMessage(QJsonObject{
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "tools/call"},
            {"params", QJsonObject{{"name", tool}, {"arguments", args}}},
        });
    EXPECT_TRUE(response.has_value());
    return response.value_or(QJsonObject()).value("result").toObject();
  }

  QJsonObject openGerberTestProject() {
    return call("project_open",
                {{"path", TEST_DATA_DIR "/projects/Gerber Test/project.lpp"},
                 {"readOnly", true}});
  }

  static QString errorText(const QJsonObject& result) {
    EXPECT_TRUE(result.value("isError").toBool(false));
    return result.value("content")
        .toArray()
        .at(0)
        .toObject()
        .value("text")
        .toString();
  }

  static QByteArray imageData(const QJsonObject& result) {
    EXPECT_FALSE(result.value("isError").toBool(true));
    const QJsonObject content =
        result.value("content").toArray().at(0).toObject();
    EXPECT_EQ("image", content.value("type").toString());
    EXPECT_EQ("image/png", content.value("mimeType").toString());
    return QByteArray::fromBase64(content.value("data").toString().toLatin1());
  }

  DesignSession mSession;
  McpServer mServer;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ProjectToolsTest, testNoProjectOpen) {
  EXPECT_TRUE(
      errorText(call("project_summary")).contains("No project is open"));
}

TEST_F(ProjectToolsTest, testOpenNonExistingProject) {
  EXPECT_TRUE(errorText(call("project_open", {{"path", "/nope/nope.lpp"}}))
                  .contains("does not exist"));
  EXPECT_FALSE(mSession.hasProject());
}

TEST_F(ProjectToolsTest, testOpenProject) {
  const QJsonObject result = openGerberTestProject();
  EXPECT_FALSE(result.value("isError").toBool(true));
  const QJsonObject summary = result.value("structuredContent").toObject();
  EXPECT_EQ(4, summary.value("partCount").toInt());
  EXPECT_EQ(12, summary.value("netCount").toInt());
  EXPECT_EQ(QJsonArray{"default"}, summary.value("boards").toArray());
  EXPECT_FALSE(summary.value("unsavedChanges").toBool(true));
}

TEST_F(ProjectToolsTest, testCloseProject) {
  openGerberTestProject();
  EXPECT_FALSE(call("project_close").value("isError").toBool(true));
  EXPECT_FALSE(mSession.hasProject());
}

TEST_F(ProjectToolsTest, testSaveReadOnlyProjectFails) {
  openGerberTestProject();
  EXPECT_TRUE(errorText(call("project_save")).contains("read-only"));
}

TEST_F(ProjectToolsTest, testPartsList) {
  openGerberTestProject();
  const QJsonArray parts = call("parts_list")
                               .value("structuredContent")
                               .toObject()
                               .value("parts")
                               .toArray();
  QStringList designators;
  for (const QJsonValue& part : parts) {
    designators.append(part.toObject().value("designator").toString());
  }
  EXPECT_EQ((QStringList{"D1", "D2", "GND1", "VCC1"}), designators);
  const QJsonObject d1 = parts.at(0).toObject();
  EXPECT_EQ("Hello", d1.value("value").toString());
  EXPECT_FALSE(d1.value("symbols").toArray().isEmpty());
}

TEST_F(ProjectToolsTest, testNetlist) {
  openGerberTestProject();
  const QJsonArray nets = call("netlist_get")
                              .value("structuredContent")
                              .toObject()
                              .value("nets")
                              .toArray();
  ASSERT_EQ(12, nets.count());
  QStringList names;
  for (const QJsonValue& net : nets) {
    names.append(net.toObject().value("name").toString());
  }
  EXPECT_TRUE(names.contains("GND"));
  EXPECT_TRUE(names.indexOf("N2") < names.indexOf("N7"));
}

TEST_F(ProjectToolsTest, testErc) {
  openGerberTestProject();
  const QJsonObject erc = call("erc_run").value("structuredContent").toObject();
  EXPECT_EQ(erc.value("messages").toArray().count(),
            erc.value("approvedCount").toInt() +
                erc.value("nonApprovedCount").toInt());
}

TEST_F(ProjectToolsTest, testDrc) {
  openGerberTestProject();
  const QJsonObject result = call("drc_run", {{"board", "default"}});
  EXPECT_FALSE(result.value("isError").toBool(true));
  EXPECT_TRUE(
      result.value("structuredContent").toObject().contains("messages"));
}

TEST_F(ProjectToolsTest, testDrcUnknownBoard) {
  openGerberTestProject();
  EXPECT_TRUE(
      errorText(call("drc_run", {{"board", "nope"}})).contains("not found"));
}

TEST_F(ProjectToolsTest, testRenderSchematic) {
  openGerberTestProject();
  const QByteArray png = imageData(call("render_schematic", {{"dpi", 20}}));
  EXPECT_TRUE(png.startsWith("\x89PNG"));
}

TEST_F(ProjectToolsTest, testRenderBoard) {
  openGerberTestProject();
  const QByteArray png =
      imageData(call("render_board", {{"side", "bottom"}, {"dpi", 50}}));
  EXPECT_TRUE(png.startsWith("\x89PNG"));
}

TEST_F(ProjectToolsTest, testRenderInvalidDpi) {
  openGerberTestProject();
  EXPECT_TRUE(errorText(call("render_board", {{"dpi", 5000}}))
                  .contains("'dpi' must be between 10 and 1200"));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace automation
}  // namespace librepcb
