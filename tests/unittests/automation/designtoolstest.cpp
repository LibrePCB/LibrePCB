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
#include <librepcb/automation/boardtools.h>
#include <librepcb/automation/designsession.h>
#include <librepcb/automation/librarytools.h>
#include <librepcb/automation/mcpserver.h>
#include <librepcb/automation/projecttools.h>
#include <librepcb/automation/schematictools.h>
#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/workspace/workspace.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class DesignToolsTest : public ::testing::Test {
public:
  DesignToolsTest()
    : mTmpDir(Application::getRandomTempPath()),
      mSession(new DesignSession()),
      mServer(new McpServer("librepcb", "1.2.3")) {
    addLibraryTools(*mServer, *mSession);
    addProjectTools(*mServer, *mSession);
    addSchematicTools(*mServer, *mSession);
    addBoardTools(*mServer, *mSession);

    // Workspace with a populated local library. The workspace is opened
    // again after adding the library to scan it.
    const FilePath wsDir = mTmpDir.getPathTo("workspace");
    structured("workspace_open",
               {{"path", wsDir.toStr()}, {"userName", "Tester"}});
    FileUtils::copyDirRecursively(
        FilePath(TEST_DATA_DIR "/libraries/Populated Library.lplib"),
        mSession->getWorkspace().getLocalLibrariesPath().getPathTo(
            "Populated Library.lplib"));
    structured("workspace_open", {{"path", wsDir.toStr()}});
  }

  ~DesignToolsTest() override {
    mServer.reset();
    mSession.reset();
    FileUtils::removeDirRecursively(mTmpDir);
  }

  QJsonObject call(const QString& tool,
                   const QJsonObject& args = QJsonObject()) {
    const std::optional<QJsonObject> response =
        mServer->handleMessage(QJsonObject{
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "tools/call"},
            {"params", QJsonObject{{"name", tool}, {"arguments", args}}},
        });
    EXPECT_TRUE(response.has_value());
    return response.value_or(QJsonObject()).value("result").toObject();
  }

  QJsonObject structured(const QString& tool,
                         const QJsonObject& args = QJsonObject()) {
    const QJsonObject result = call(tool, args);
    EXPECT_FALSE(result.value("isError").toBool(true))
        << tool.toStdString() << ": " << text(result).toStdString();
    return result.value("structuredContent").toObject();
  }

  void ok(const QString& tool, const QJsonObject& args = QJsonObject()) {
    const QJsonObject result = call(tool, args);
    EXPECT_FALSE(result.value("isError").toBool(true))
        << tool.toStdString() << ": " << text(result).toStdString();
  }

  QString error(const QString& tool, const QJsonObject& args = QJsonObject()) {
    const QJsonObject result = call(tool, args);
    EXPECT_TRUE(result.value("isError").toBool(false)) << tool.toStdString();
    return text(result);
  }

  static QString text(const QJsonObject& result) {
    return result.value("content")
        .toArray()
        .at(0)
        .toObject()
        .value("text")
        .toString();
  }

  /**
   * @brief Project with R1 and R2, nets A (R1.1, R2.1) and B (R1.2, R2.2)
   */
  void createProjectWithTwoResistors() {
    ok("project_create",
       {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
        {"name", "Test"}});
    for (const char* value : {"1k", "47k"}) {
      structured("schematic_add_part",
                 {{"component", "Resistor"},
                  {"device", "R-0805"},
                  {"attributes", QJsonObject{{"RESISTANCE", value}}},
                  {"position", QJsonArray{50, 50}}});
    }
    structured("schematic_connect",
               {{"pins", QJsonArray{"R1.1", "R2.1"}}, {"net", "A"}});
    structured("schematic_connect",
               {{"pins", QJsonArray{"R1.2", "R2.2"}}, {"net", "B"}});
  }

  QMap<QString, QStringList> netlist() {
    QMap<QString, QStringList> result;
    const QJsonArray nets = structured("netlist_get").value("nets").toArray();
    for (const QJsonValue& net : nets) {
      QStringList pins;
      const QJsonArray pinValues = net.toObject().value("pins").toArray();
      for (const QJsonValue& pin : pinValues) {
        pins.append(pin.toString());
      }
      result.insert(net.toObject().value("name").toString(), pins);
    }
    return result;
  }

  int unroutedCount() {
    return structured("board_status")
        .value("unroutedConnections")
        .toArray()
        .count();
  }

  FilePath mTmpDir;
  std::unique_ptr<DesignSession> mSession;
  std::unique_ptr<McpServer> mServer;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(DesignToolsTest, testLibrarySearchAndGet) {
  const QJsonArray results =
      structured("library_search", {{"query", "resist"}, {"type", "component"}})
          .value("results")
          .toArray();
  ASSERT_EQ(1, results.count());
  EXPECT_EQ("Resistor", results.at(0).toObject().value("name").toString());

  const QJsonObject cmp = structured(
      "library_get", {{"type", "component"}, {"element", "Resistor"}});
  EXPECT_EQ("R", cmp.value("prefix").toString());
  EXPECT_EQ((QJsonArray{"1", "2"}), cmp.value("signals").toArray());
  EXPECT_EQ(3, cmp.value("devices").toArray().count());
}

TEST_F(DesignToolsTest, testLibraryElementNotFoundListsSimilarNames) {
  const QString msg =
      error("library_get", {{"type", "device"}, {"element", "R-08"}});
  EXPECT_TRUE(msg.contains("'R-0805'")) << msg.toStdString();
}

TEST_F(DesignToolsTest, testCreateLibraryElements) {
  structured("library_create",
             {{"name", "Custom Parts"},
              {"dependencies", QJsonArray{"Populated Library"}}});
  const QJsonObject sym =
      structured("symbol_create",
                 {{"library", "Custom Parts"},
                  {"name", "Two Pins"},
                  {"categories", QJsonArray{"Resistors"}},
                  {"pins",
                   QJsonArray{
                       QJsonObject{{"name", "A"},
                                   {"position", QJsonArray{-7.62, 0}},
                                   {"rotation", 0}},
                       QJsonObject{{"name", "B"},
                                   {"position", QJsonArray{7.62, 0}},
                                   {"rotation", 180}},
                   }},
                  {"bodyFrom", QJsonArray{-5.08, 2.54}},
                  {"bodyTo", QJsonArray{5.08, -2.54}}});
  EXPECT_EQ(0, sym.value("nonApprovedCount").toInt())
      << QJsonDocument(sym).toJson().toStdString();
  const QJsonObject cmp = structured("component_create",
                                     {{"library", "Custom Parts"},
                                      {"name", "Two Pins"},
                                      {"symbol", "Two Pins"},
                                      {"prefix", "X"}});
  EXPECT_FALSE(cmp.value("uuid").toString().isEmpty());
  structured(
      "device_create",
      {{"library", "Custom Parts"},
       {"name", "Two Pins 0805"},
       {"component", "Two Pins"},
       {"package", "RESC2012 (0805)"},
       {"padSignalMap", QJsonObject{{"1", "A"}, {"2", "B"}}},
       {"parts",
        QJsonArray{QJsonObject{{"mpn", "TP-1"}, {"manufacturer", "ACME"}}}}});
  const QJsonObject dev = structured(
      "library_get", {{"type", "device"}, {"element", "Two Pins 0805"}});
  EXPECT_EQ((QJsonObject{{"1", "A"}, {"2", "B"}}),
            dev.value("padSignalMap").toObject());
  EXPECT_EQ(
      "TP-1",
      dev.value("parts").toArray().at(0).toObject().value("mpn").toString());
}

TEST_F(DesignToolsTest, testPartValueUsesMpnOfChosenDevice) {
  structured("library_create", {{"name", "Custom Parts"}});
  structured(
      "symbol_create",
      {{"library", "Custom Parts"},
       {"name", "One Pin"},
       {"categories", QJsonArray{"Resistors"}},
       {"pins",
        QJsonArray{QJsonObject{
            {"name", "A"}, {"position", QJsonArray{0, 0}}, {"rotation", 0}}}},
       {"bodyFrom", QJsonArray{2.54, 2.54}},
       {"bodyTo", QJsonArray{7.62, -2.54}}});
  structured("component_create",
             {{"library", "Custom Parts"},
              {"name", "One Pin"},
              {"symbol", "One Pin"},
              {"prefix", "X"},
              {"defaultValue", "{{MPN or DEVICE}}"}});
  structured("device_create",
             {{"library", "Custom Parts"},
              {"name", "One Pin 0805"},
              {"component", "One Pin"},
              {"package", "RESC2012 (0805)"},
              {"padSignalMap", QJsonObject{{"1", "A"}}},
              {"parts", QJsonArray{QJsonObject{{"mpn", "OP-1"}}}}});
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  EXPECT_EQ("OP-1",
            structured("schematic_add_part",
                       {{"component", "One Pin"},
                        {"device", "One Pin 0805"},
                        {"position", QJsonArray{0, 0}}})
                .value("value")
                .toString());
  EXPECT_EQ("OP-1",
            structured("parts_list")
                .value("parts")
                .toArray()
                .at(0)
                .toObject()
                .value("value")
                .toString());
}

TEST_F(DesignToolsTest, testCreateDeviceWithUnknownSignalFails) {
  structured("library_create", {{"name", "Custom Parts"}});
  EXPECT_TRUE(error("device_create",
                    {{"library", "Custom Parts"},
                     {"name", "Bad"},
                     {"component", "Resistor"},
                     {"package", "RESC2012 (0805)"},
                     {"padSignalMap", QJsonObject{{"1", "X"}}}})
                  .contains("no signal named 'X'"));
}

TEST_F(DesignToolsTest, testSchematicAddPart) {
  createProjectWithTwoResistors();
  const QJsonObject part = structured("part_get", {{"designator", "R2"}});
  EXPECT_EQ("47kΩ", part.value("value").toString());
  EXPECT_EQ(2,
            part.value("symbols")
                .toArray()
                .at(0)
                .toObject()
                .value("pins")
                .toArray()
                .count());
}

TEST_F(DesignToolsTest, testSchematicConnectIsErcClean) {
  createProjectWithTwoResistors();
  const QMap<QString, QStringList> nets = netlist();
  EXPECT_EQ((QStringList{"R1.1", "R2.1"}), nets.value("A"));
  EXPECT_EQ((QStringList{"R1.2", "R2.2"}), nets.value("B"));
  EXPECT_EQ(0, structured("erc_run").value("messages").toArray().count());
}

TEST_F(DesignToolsTest, testConnectPinsOfDifferentNetsFails) {
  createProjectWithTwoResistors();
  EXPECT_TRUE(error("schematic_connect", {{"pins", QJsonArray{"R1.1", "R1.2"}}})
                  .contains("different nets"));
}

TEST_F(DesignToolsTest, testConnectUnknownPinListsAvailablePins) {
  createProjectWithTwoResistors();
  EXPECT_TRUE(error("schematic_connect", {{"pins", QJsonArray{"R1.X"}}})
                  .contains("Available: 1, 2"));
}

TEST_F(DesignToolsTest, testDisconnectAndUndo) {
  createProjectWithTwoResistors();
  ok("schematic_disconnect", {{"pins", QJsonArray{"R1.1"}}});
  EXPECT_EQ((QStringList{"R2.1"}), netlist().value("A"));
  ok("undo");
  EXPECT_EQ((QStringList{"R1.1", "R2.1"}), netlist().value("A"));
  ok("redo");
  EXPECT_EQ((QStringList{"R2.1"}), netlist().value("A"));
}

TEST_F(DesignToolsTest, testRenameNet) {
  createProjectWithTwoResistors();
  ok("net_rename", {{"net", "A"}, {"name", "SPI_SCK"}});
  EXPECT_EQ((QStringList{"R1.1", "R2.1"}), netlist().value("SPI_SCK"));
  EXPECT_TRUE(error("net_rename", {{"net", "SPI_SCK"}, {"name", "B"}})
                  .contains("exists already"));
}

TEST_F(DesignToolsTest, testMovePartKeepsConnections) {
  createProjectWithTwoResistors();
  const QJsonObject part = structured("schematic_move_part",
                                      {{"designator", "R1"},
                                       {"position", QJsonArray{80, 50}},
                                       {"rotation", 90}});
  const QJsonObject symbol = part.value("symbols").toArray().at(0).toObject();
  EXPECT_EQ(QJsonArray({80, 50}), symbol.value("position").toArray());
  EXPECT_EQ((QStringList{"R1.1", "R2.1"}), netlist().value("A"));
  EXPECT_EQ(0, structured("erc_run").value("messages").toArray().count());
}

TEST_F(DesignToolsTest, testRemovePart) {
  createProjectWithTwoResistors();
  ok("part_remove", {{"designator", "R2"}});
  EXPECT_TRUE(error("part_get", {{"designator", "R2"}}).contains("No part"));
  EXPECT_EQ((QStringList{"R1.1"}), netlist().value("A"));
}

TEST_F(DesignToolsTest, testBoardPlaceRouteAndPlane) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  const QJsonObject r1 = structured(
      "board_place_device",
      {{"designator", "R1"}, {"position", QJsonArray{10, 5}}, {"rotation", 0}});
  EXPECT_EQ("R-0805", r1.value("device").toString());
  EXPECT_EQ(2, r1.value("pads").toArray().count());
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  EXPECT_EQ(2, unroutedCount());

  ok("board_route",
     {{"from", "R1.1"},
      {"to", "R2.1"},
      {"layer", "top"},
      {"points", QJsonArray{QJsonValue(QJsonArray{5, 10})}}});
  EXPECT_EQ(1, unroutedCount());
  EXPECT_TRUE(
      error("board_route", {{"from", "R1.1"}, {"to", "R2.2"}, {"layer", "top"}})
          .contains("different nets"));
  EXPECT_TRUE(error("board_route",
                    {{"from", "R1.2"}, {"to", "R2.2"}, {"layer", "bottom"}})
                  .contains("not on the"));

  ok("board_add_plane", {{"net", "B"}, {"layer", "top"}});
  EXPECT_EQ(0, unroutedCount());
  EXPECT_TRUE(structured("drc_run").contains("messages"));
}

TEST_F(DesignToolsTest, testBoardPlaceWithoutDeviceListsDevices) {
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  structured("schematic_add_part",
             {{"component", "Resistor"}, {"position", QJsonArray{0, 0}}});
  EXPECT_TRUE(error("board_place_device",
                    {{"designator", "R1"}, {"position", QJsonArray{0, 0}}})
                  .contains("'R-0603'"));
}

TEST_F(DesignToolsTest, testBoardRouteThroughVia) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  const QString via =
      structured("board_add_via",
                 {{"net", "A"}, {"position", QJsonArray{5, 10}}})
          .value("via")
          .toString();
  ok("board_route", {{"from", "R1.1"}, {"to", via}, {"layer", "top"}});
  ok("board_route", {{"from", via}, {"to", "R2.1"}, {"layer", "top"}});
  EXPECT_EQ(1, unroutedCount());
}

TEST_F(DesignToolsTest, testOutputJobs) {
  createProjectWithTwoResistors();
  const QJsonObject result = structured("output_jobs_run");
  const QJsonArray files = result.value("files").toArray();
  EXPECT_TRUE(files.contains("gerber/Test_v1_COPPER-TOP.gbr"))
      << QJsonDocument(result).toJson().toStdString();
}

TEST_F(DesignToolsTest, testAddPartRequiresChoiceOfPart) {
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  const QString msg = error("schematic_add_part",
                            {{"component", "Demo Component"},
                             {"device", "Demo Device"},
                             {"position", QJsonArray{0, 0}}});
  EXPECT_TRUE(msg.contains("'DEMO1', 'DEMO2', 'DEMO3'")) << msg.toStdString();
  EXPECT_EQ("DEMO2",
            structured("schematic_add_part",
                       {{"component", "Demo Component"},
                        {"device", "Demo Device"},
                        {"part", "DEMO2"},
                        {"position", QJsonArray{0, 0}}})
                .value("mpn")
                .toString());
  EXPECT_TRUE(structured("schematic_add_part",
                         {{"component", "Demo Component"},
                          {"device", "Demo Device"},
                          {"part", QJsonValue::Null},
                          {"position", QJsonArray{20, 0}}})
                  .value("mpn")
                  .isNull());
}

TEST_F(DesignToolsTest, testChangePartOfPart) {
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  structured("schematic_add_part",
             {{"component", "Demo Component"},
              {"device", "Demo Device"},
              {"part", "DEMO2"},
              {"position", QJsonArray{0, 0}}});
  EXPECT_EQ("DEMO3",
            structured("part_set_properties",
                       {{"designator", "D1"}, {"part", "DEMO3"}})
                .value("mpn")
                .toString());
  EXPECT_TRUE(
      error("part_set_properties", {{"designator", "D1"}, {"part", "X"}})
          .contains("'DEMO1'"));
}

TEST_F(DesignToolsTest, testAttributeValueWithLongUnit) {
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  const QJsonObject part =
      structured("schematic_add_part",
                 {{"component", "Crystal"},
                  {"device", "Crystal ABM3"},
                  {"attributes", QJsonObject{{"FREQUENCY", "16MHz"}}},
                  {"position", QJsonArray{0, 0}}});
  EXPECT_TRUE(part.value("value").toString().contains("16MHz"))
      << part.value("value").toString().toStdString();
}

TEST_F(DesignToolsTest, testMovePartKeepsBoardTraces) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  ok("board_route", {{"from", "R1.1"}, {"to", "R2.1"}, {"layer", "top"}});
  ok("board_route", {{"from", "R1.2"}, {"to", "R2.2"}, {"layer", "top"}});
  ASSERT_EQ(0, unroutedCount());
  structured("schematic_move_part",
             {{"designator", "R1"},
              {"position", QJsonArray{80, 50}},
              {"rotation", 90}});
  EXPECT_EQ(0, unroutedCount());
  EXPECT_EQ(2, mSession->getBoard(0).getNetSegments().count());
  EXPECT_EQ(0, structured("erc_run").value("messages").toArray().count());
}

TEST_F(DesignToolsTest, testMovePartMovesNetLabels) {
  createProjectWithTwoResistors();
  const QJsonObject part = structured("schematic_move_part",
                                      {{"designator", "R1"},
                                       {"position", QJsonArray{80, 50}},
                                       {"rotation", 90}});
  Schematic& schematic = mSession->getSchematic(0);
  const QJsonArray positions =
      part.value("symbols").toArray().at(0).toObject().value("pins").toArray();
  for (const QJsonValue& value : positions) {
    const Point pin = Point::fromMm(
        value.toObject().value("position").toArray().at(0).toDouble(),
        value.toObject().value("position").toArray().at(1).toDouble());
    int nearbyLabels = 0;
    for (const SI_NetSegment* segment : schematic.getNetSegments()) {
      for (const SI_NetLabel* label : segment->getNetLabels()) {
        if ((label->getPosition() - pin).getLength() <= Length(2540000)) {
          ++nearbyLabels;
        }
      }
    }
    EXPECT_EQ(1, nearbyLabels);
  }
}

TEST_F(DesignToolsTest, testViaReferenceStaysValidAfterJoiningSegments) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  const QString via1 =
      structured("board_add_via",
                 {{"net", "A"}, {"position", QJsonArray{5, 8}}})
          .value("via")
          .toString();
  const QString via2 =
      structured("board_add_via",
                 {{"net", "A"}, {"position", QJsonArray{5, 12}}})
          .value("via")
          .toString();
  ok("board_route", {{"from", via1}, {"to", via2}, {"layer", "bottom"}});
  ok("board_route", {{"from", "R1.1"}, {"to", via1}, {"layer", "top"}});
  ok("board_route", {{"from", via2}, {"to", "R2.1"}, {"layer", "top"}});
  EXPECT_EQ(1, unroutedCount());
}

TEST_F(DesignToolsTest, testElementOfOtherTypeIsHinted) {
  const QString msg =
      error("library_get", {{"type", "component"}, {"element", "R-0805"}});
  EXPECT_TRUE(msg.contains("'R-0805' is a device")) << msg.toStdString();
}

TEST_F(DesignToolsTest, testLibrarySearchWithEmptyQueryAndPaging) {
  const QJsonObject all =
      structured("library_search", {{"query", ""}, {"type", "component"}});
  EXPECT_EQ(15, all.value("total").toInt());
  EXPECT_EQ(15, all.value("results").toArray().count());
  const QJsonObject page = structured(
      "library_search",
      {{"query", ""}, {"type", "component"}, {"offset", 10}, {"limit", 3}});
  EXPECT_EQ(15, page.value("total").toInt());
  ASSERT_EQ(3, page.value("results").toArray().count());
  EXPECT_EQ(all.value("results").toArray().at(10),
            page.value("results").toArray().at(0));
}

TEST_F(DesignToolsTest, testLibrarySearchFindsDevicesByPartNumber) {
  const QJsonArray results =
      structured("library_search", {{"query", "DEMO2"}, {"type", "device"}})
          .value("results")
          .toArray();
  ASSERT_EQ(1, results.count());
  EXPECT_EQ("Demo Device", results.at(0).toObject().value("name").toString());
}

TEST_F(DesignToolsTest, testRenderBoardReturnsImageTransform) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  const QJsonObject result = call("render_board", {{"dpi", 254}});
  ASSERT_FALSE(result.value("isError").toBool(true));
  const QJsonObject info = result.value("structuredContent").toObject();
  EXPECT_DOUBLE_EQ(10.0, info.value("pixelsPerMm").toDouble());
  const QJsonArray topLeft = info.value("topLeft").toArray();
  // The board center is in the image center, independent of margins.
  EXPECT_NEAR(info.value("width").toDouble() / 2,
              (15 - topLeft.at(0).toDouble()) * 10, 2.0);
  EXPECT_NEAR(info.value("height").toDouble() / 2,
              (topLeft.at(1).toDouble() - 10) * 10, 2.0);
  bool hasImage = false;
  const QJsonArray contents = result.value("content").toArray();
  for (const QJsonValue& content : contents) {
    hasImage |= (content.toObject().value("type").toString() == "image");
  }
  EXPECT_TRUE(hasImage);
}

static QJsonArray pts(std::initializer_list<std::pair<qreal, qreal>> points) {
  QJsonArray array;
  for (const auto& [x, y] : points) {
    array.append(QJsonArray{x, y});
  }
  return array;
}

static bool fragmentsContain(const BI_Plane& plane, const Point& p) {
  for (const Path& fragment : plane.getFragments()) {
    if (fragment.toQPainterPathPx().contains(p.toPxQPointF())) {
      return true;
    }
  }
  return false;
}

TEST_F(DesignToolsTest, testPolygonOutlineWithArc) {
  createProjectWithTwoResistors();
  ok("board_set_outline",
     {{"vertices",
       QJsonArray{QJsonArray{0, 0}, QJsonArray{30, 0, 90}, QJsonArray{30, 10},
                  QJsonArray{20, 20}, QJsonArray{0, 20}}}});
  const BI_Polygon* outline = nullptr;
  for (const BI_Polygon* polygon : mSession->getBoard(0).getPolygons()) {
    if (polygon->getData().getLayer() == Layer::boardOutlines()) {
      outline = polygon;
    }
  }
  ASSERT_NE(nullptr, outline);
  const Path& path = outline->getData().getPath();
  EXPECT_TRUE(path.isClosed());
  ASSERT_EQ(6, path.getVertices().count());
  EXPECT_EQ(Angle::deg90(), path.getVertices().at(1).getAngle());
  EXPECT_EQ(Point::fromMm(20, 20), path.getVertices().at(3).getPos());
  EXPECT_TRUE(
      error("board_set_outline",
            {{"width", 10}, {"vertices", pts({{0, 0}, {1, 0}, {1, 1}})}})
          .contains("either"));
  EXPECT_TRUE(error("board_set_outline", {{"vertices", pts({{0, 0}, {1, 0}})}})
                  .contains("at least 3"));
}

TEST_F(DesignToolsTest, testBoardCutouts) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  ok("board_add_cutout", {{"vertices", pts({{5, 5}, {8, 5}, {8, 8}, {5, 8}})}});
  ok("board_add_cutout", {{"center", QJsonArray{20, 10}}, {"diameter", 3}});
  int count = 0;
  for (const BI_Polygon* polygon : mSession->getBoard(0).getPolygons()) {
    if (polygon->getData().getLayer() == Layer::boardCutouts()) {
      EXPECT_TRUE(polygon->getData().getPath().isClosed());
      ++count;
    }
  }
  EXPECT_EQ(2, count);
  EXPECT_TRUE(error("board_add_cutout", {{"center", QJsonArray{20, 10}}})
                  .contains("diameter"));
}

TEST_F(DesignToolsTest, testPlaneAreaAndKeepout) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{8, 15}}});
  ok("board_add_plane",
     {{"net", "A"},
      {"layer", "top"},
      {"vertices", pts({{0, 0}, {15, 0}, {15, 20}, {0, 20}})},
      {"priority", 2},
      {"connectStyle", "solid"}});
  ok("board_add_keepout",
     {{"vertices", pts({{2, 2}, {6, 2}, {6, 6}, {2, 6}})},
      {"layers", "both"},
      {"rules", QJsonArray{"noCopper", "noPlanes"}}});
  Board& board = mSession->getBoard(0);
  ASSERT_EQ(1, board.getPlanes().count());
  const BI_Plane& plane = *board.getPlanes().first();
  EXPECT_EQ(2, plane.getPriority());
  EXPECT_EQ(BI_Plane::ConnectStyle::Solid, plane.getConnectStyle());
  ASSERT_EQ(1, board.getZones().count());
  const BI_Zone& zone = *board.getZones().first();
  EXPECT_EQ((QSet<const Layer*>{&Layer::topCopper(), &Layer::botCopper()}),
            zone.getData().getLayers());
  EXPECT_TRUE(zone.getData().getRules().testFlag(Zone::Rule::NoPlanes));
  EXPECT_FALSE(zone.getData().getRules().testFlag(Zone::Rule::NoDevices));

  unroutedCount();  // Fills the planes.
  EXPECT_TRUE(fragmentsContain(plane, Point::fromMm(10, 5)));
  EXPECT_FALSE(fragmentsContain(plane, Point::fromMm(4, 4)));
  EXPECT_FALSE(fragmentsContain(plane, Point::fromMm(20, 10)));
}

TEST_F(DesignToolsTest, testBoardTexts) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  ok("board_add_text",
     {{"text", "Hello"},
      {"position", QJsonArray{5, 15}},
      {"layer", "bot_legend"},
      {"height", 1.5}});
  Board& board = mSession->getBoard(0);
  ASSERT_EQ(1, board.getStrokeTexts().count());
  const BI_StrokeText& text = *board.getStrokeTexts().first();
  EXPECT_EQ("Hello", text.getData().getText());
  EXPECT_EQ(&Layer::botLegend(), &text.getData().getLayer());
  EXPECT_TRUE(text.getData().getMirrored());
  EXPECT_EQ(PositiveLength(1500000), text.getData().getHeight());

  ok("board_set_part_text",
     {{"designator", "R1"},
      {"text", "name"},
      {"position", QJsonArray{10, 8}},
      {"rotation", 90},
      {"height", 0.8}});
  ok("board_set_part_text",
     {{"designator", "R1"}, {"text", "value"}, {"visible", false}});
  auto deviceTexts = [&]() {
    QMap<QString, const BI_StrokeText*> texts;
    for (const BI_StrokeText* t :
         board.getDeviceInstances().first()->getStrokeTexts()) {
      texts.insert(t->getData().getText(), t);
    }
    return texts;
  };
  ASSERT_TRUE(deviceTexts().contains("{{NAME}}"));
  EXPECT_FALSE(deviceTexts().contains("{{VALUE}}"));
  const BoardStrokeTextData& name = deviceTexts().value("{{NAME}}")->getData();
  EXPECT_EQ(Point::fromMm(10, 8), name.getPosition());
  EXPECT_EQ(Angle::deg90(), name.getRotation());
  EXPECT_EQ(PositiveLength(800000), name.getHeight());
  ok("board_set_part_text",
     {{"designator", "R1"}, {"text", "value"}, {"visible", true}});
  EXPECT_TRUE(deviceTexts().contains("{{VALUE}}"));
}

TEST_F(DesignToolsTest, testCreateSmtPackage) {
  structured("library_create",
             {{"name", "Custom Parts"},
              {"dependencies", QJsonArray{"Populated Library"}}});
  auto pad = [](const QString& name, qreal x, qreal y) {
    return QJsonObject{{"name", name},
                       {"position", QJsonArray{x, y}},
                       {"size", QJsonArray{0.8, 0.5}}};
  };
  const QJsonObject result =
      structured("package_create",
                 {{"library", "Custom Parts"},
                  {"name", "Test QFN"},
                  {"categories", QJsonArray{"Crystals"}},
                  {"assemblyType", "smt"},
                  {"pads",
                   QJsonArray{pad("1", -1.5, -0.5), pad("2", 1.5, -0.5),
                              pad("3", 1.5, 0.5), pad("4", -1.5, 0.5)}},
                  {"body", QJsonArray{3, 3}}});
  const QJsonArray messages = result.value("messages").toArray();
  for (const QJsonValue& msg : messages) {
    EXPECT_NE("error", msg.toObject().value("severity").toString())
        << msg.toObject().value("message").toString().toStdString();
  }
  const QJsonObject pkg =
      structured("library_get", {{"type", "package"}, {"element", "Test QFN"}});
  const QJsonArray pads = pkg.value("footprintPads").toArray();
  ASSERT_EQ(4, pads.count());
  const QJsonObject pad1 = pads.at(0).toObject();
  EXPECT_EQ("1", pad1.value("name").toString());
  EXPECT_EQ(QJsonArray({-1.5, -0.5}), pad1.value("position").toArray());
  EXPECT_EQ(QJsonArray({0.8, 0.5}), pad1.value("size").toArray());
  EXPECT_EQ("top", pad1.value("side").toString());
  EXPECT_TRUE(pad1.value("drill").isNull());
}

TEST_F(DesignToolsTest, testCreateThtPackageAndPlaceIt) {
  structured("library_create",
             {{"name", "Custom Parts"},
              {"dependencies", QJsonArray{"Populated Library"}}});
  structured("package_create",
             {{"library", "Custom Parts"},
              {"name", "Header 1x2"},
              {"assemblyType", "tht"},
              {"pads",
               QJsonArray{QJsonObject{{"name", "1"},
                                      {"position", QJsonArray{-1.27, 0}},
                                      {"size", QJsonArray{1.7, 1.7}},
                                      {"shape", "rect"},
                                      {"drill", 1.0}},
                          QJsonObject{{"name", "2"},
                                      {"position", QJsonArray{1.27, 0}},
                                      {"size", QJsonArray{1.7, 1.7}},
                                      {"shape", "round"},
                                      {"drill", 1.0}}}},
              {"body", QJsonArray{5.08, 2.54}}});
  structured("device_create",
             {{"library", "Custom Parts"},
              {"name", "R Header"},
              {"component", "Resistor"},
              {"package", "Header 1x2"},
              {"padSignalMap", QJsonObject{{"1", "1"}, {"2", "2"}}}});
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  structured("schematic_add_part",
             {{"component", "Resistor"},
              {"device", "R Header"},
              {"position", QJsonArray{0, 0}}});
  const QJsonArray pads =
      structured("board_place_device",
                 {{"designator", "R1"}, {"position", QJsonArray{10, 10}}})
          .value("pads")
          .toArray();
  ASSERT_EQ(2, pads.count());
  const QJsonObject pad1 = (pads.at(0).toObject().value("pad") == "1")
      ? pads.at(0).toObject()
      : pads.at(1).toObject();
  EXPECT_EQ("both", pad1.value("layers").toString());
  const QJsonArray position = pad1.value("position").toArray();
  EXPECT_DOUBLE_EQ(8.73, position.at(0).toDouble())
      << QJsonDocument(pads).toJson().toStdString();
  EXPECT_DOUBLE_EQ(10, position.at(1).toDouble());
}

TEST_F(DesignToolsTest, testApproveDrcMessages) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  const QJsonObject before = structured("drc_run");
  const QJsonArray messages = before.value("messages").toArray();
  ASSERT_FALSE(messages.isEmpty());
  const QString id = messages.at(0).toObject().value("id").toString();
  ASSERT_FALSE(id.isEmpty());
  EXPECT_EQ(1,
            structured("drc_approve", {{"ids", QJsonArray{id}}})
                .value("changed")
                .toInt());
  ok("project_save");
  ok("project_open", {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()}});
  const QJsonObject after = structured("drc_run");
  EXPECT_EQ(before.value("nonApprovedCount").toInt() - 1,
            after.value("nonApprovedCount").toInt());
  const QJsonArray afterMessages = after.value("messages").toArray();
  for (const QJsonValue& msg : afterMessages) {
    EXPECT_EQ(msg.toObject().value("id").toString() == id,
              msg.toObject().value("approved").toBool());
  }
  ok("drc_approve", {{"ids", QJsonArray{id}}, {"approved", false}});
  EXPECT_EQ(before.value("nonApprovedCount").toInt(),
            structured("drc_run").value("nonApprovedCount").toInt());
  EXPECT_TRUE(
      error("drc_approve", {{"ids", QJsonArray{"nope"}}}).contains("'nope'"));
}

TEST_F(DesignToolsTest, testApproveErcMessages) {
  createProjectWithTwoResistors();
  ok("schematic_disconnect", {{"pins", QJsonArray{"R1.1"}}});
  const QJsonObject before = structured("erc_run");
  ASSERT_GT(before.value("nonApprovedCount").toInt(), 0);
  QJsonArray ids;
  const QJsonArray messages = before.value("messages").toArray();
  for (const QJsonValue& msg : messages) {
    ids.append(msg.toObject().value("id"));
  }
  ok("erc_approve", {{"ids", ids}});
  EXPECT_EQ(0, structured("erc_run").value("nonApprovedCount").toInt());
}

TEST_F(DesignToolsTest, testDrcReportsEveryLocationOfAMessage) {
  createProjectWithTwoResistors();
  // Two concave corners, both too sharp for the milling tool.
  ok("board_set_outline",
     {{"vertices",
       pts({{0, 0},
            {30, 0},
            {30, 20},
            {20, 20},
            {20, 10},
            {10, 10},
            {10, 20},
            {0, 20}})}});
  QJsonArray locations;
  const QJsonArray messages = structured("drc_run").value("messages").toArray();
  for (const QJsonValue& msg : messages) {
    if (msg.toObject().value("message").toString().contains("inner radius")) {
      locations = msg.toObject().value("locations").toArray();
    }
  }
  ASSERT_EQ(2, locations.count());
  QList<qreal> xs;
  for (const QJsonValue& location : std::as_const(locations)) {
    const QJsonArray center = location.toObject().value("center").toArray();
    EXPECT_NEAR(10, center.at(1).toDouble(), 1.0);
    xs.append(center.at(0).toDouble());
  }
  std::sort(xs.begin(), xs.end());
  EXPECT_NEAR(10, xs.at(0), 1.0);
  EXPECT_NEAR(20, xs.at(1), 1.0);
}

TEST_F(DesignToolsTest, testRouteReturnsTraceWidth) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  EXPECT_DOUBLE_EQ(
      mSession->getBoard(0).getDesignRules().getDefaultTraceWidth()->toMm(),
      structured("board_route",
                 {{"from", "R1.1"}, {"to", "R2.1"}, {"layer", "top"}})
          .value("width")
          .toDouble());
  EXPECT_DOUBLE_EQ(
      0.3,
      structured(
          "board_route",
          {{"from", "R1.2"}, {"to", "R2.2"}, {"layer", "top"}, {"width", 0.3}})
          .value("width")
          .toDouble());
}

TEST_F(DesignToolsTest, testCategoryOfOtherTypeIsHinted) {
  structured("library_create", {{"name", "Custom Parts"}});
  const QString msg =
      error("package_create",
            {{"library", "Custom Parts"},
             {"name", "Pad"},
             {"categories", QJsonArray{"Resistors"}},
             {"pads",
              QJsonArray{QJsonObject{{"name", "1"},
                                     {"position", QJsonArray{0, 0}},
                                     {"size", QJsonArray{1, 1}}}}}});
  EXPECT_TRUE(msg.contains("'Resistors' is a component_category"))
      << msg.toStdString();
}

TEST_F(DesignToolsTest, testPartTextAlignment) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  ok("board_set_part_text",
     {{"designator", "R1"},
      {"text", "name"},
      {"position", QJsonArray{10, 8}},
      {"align", "left"}});
  for (const BI_StrokeText* text :
       mSession->getBoard(0).getDeviceInstances().first()->getStrokeTexts()) {
    if (text->getData().getText() == "{{NAME}}") {
      EXPECT_EQ(Alignment(HAlign::left(), VAlign::bottom()),
                text->getData().getAlign());
    }
  }
}

TEST_F(DesignToolsTest, testNumberLimitsAreInSchema) {
  const QJsonArray tools = mServer
                               ->handleMessage(QJsonObject{
                                   {"jsonrpc", "2.0"},
                                   {"id", 1},
                                   {"method", "tools/list"},
                               })
                               .value_or(QJsonObject())
                               .value("result")
                               .toObject()
                               .value("tools")
                               .toArray();
  QJsonObject dpi;
  for (const QJsonValue& tool : tools) {
    if (tool.toObject().value("name") == "render_board") {
      dpi = tool.toObject()
                .value("inputSchema")
                .toObject()
                .value("properties")
                .toObject()
                .value("dpi")
                .toObject();
    }
  }
  EXPECT_EQ(10, dpi.value("minimum").toInt());
  EXPECT_EQ(1200, dpi.value("maximum").toInt());
}

static Point padPosition(const QJsonObject& device, const QString& pad) {
  const QJsonArray pads = device.value("pads").toArray();
  for (const QJsonValue& value : pads) {
    if (value.toObject().value("pad").toString() == pad) {
      const QJsonArray pos = value.toObject().value("position").toArray();
      return Point::fromMm(pos.at(0).toDouble(), pos.at(1).toDouble());
    }
  }
  ADD_FAILURE() << "Pad " << pad.toStdString() << " not found.";
  return Point();
}

static QJsonArray toArray(const Point& p) {
  return QJsonArray{p.getX().toMm(), p.getY().toMm()};
}

static QStringList ids(const QJsonArray& items) {
  QStringList result;
  for (const QJsonValue& item : items) {
    result.append(item.toObject().value("id").toString());
  }
  return result;
}

TEST_F(DesignToolsTest, testListBoardItems) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  const QString via =
      structured("board_add_via",
                 {{"net", "A"}, {"position", QJsonArray{5, 10}}})
          .value("via")
          .toString();
  const QJsonArray traces =
      structured("board_route",
                 {{"from", "R1.1"}, {"to", via}, {"layer", "top"}})
          .value("traces")
          .toArray();
  ASSERT_FALSE(traces.isEmpty());
  const QString text =
      structured("board_add_text",
                 {{"text", "Hi"}, {"position", QJsonArray{25, 18}}})
          .value("text")
          .toString();
  const QString keepout =
      structured("board_add_keepout",
                 {{"vertices", pts({{20, 2}, {24, 2}, {24, 6}, {20, 6}})}})
          .value("keepout")
          .toString();
  const QString plane =
      structured("board_add_plane", {{"net", "B"}, {"layer", "bottom"}})
          .value("plane")
          .toString();
  const QString cutout =
      structured("board_add_cutout",
                 {{"center", QJsonArray{27, 3}}, {"diameter", 2}})
          .value("polygon")
          .toString();
  const QString hole =
      structured("board_add_hole",
                 {{"position", QJsonArray{3, 17}}, {"diameter", 3.2}})
          .value("hole")
          .toString();

  const QJsonObject all = structured("board_list_items");
  EXPECT_FALSE(all.contains("pads"));  // Only on request, there are many.
  EXPECT_FALSE(all.contains("points"));
  EXPECT_EQ(QStringList{traces.at(0).toString()},
            ids(all.value("traces").toArray()));
  const QJsonObject trace = all.value("traces").toArray().at(0).toObject();
  EXPECT_EQ("A", trace.value("net").toString());
  EXPECT_EQ("top", trace.value("layer").toString());
  EXPECT_TRUE(trace.value("from").toObject().contains("pad") ||
              trace.value("to").toObject().contains("pad"));
  EXPECT_EQ(QStringList{via}, ids(all.value("vias").toArray()));
  QStringList boardTexts;
  const QJsonArray texts = all.value("texts").toArray();
  for (const QJsonValue& t : texts) {
    if (!t.toObject().contains("part")) {
      boardTexts.append(t.toObject().value("id").toString());
    }
  }
  EXPECT_EQ(QStringList{text}, boardTexts);
  EXPECT_GT(all.value("texts").toArray().count(), 1);  // Name/value of parts.
  EXPECT_EQ(QStringList{keepout}, ids(all.value("keepouts").toArray()));
  EXPECT_EQ(QStringList{plane}, ids(all.value("planes").toArray()));
  EXPECT_EQ(QStringList{hole}, ids(all.value("holes").toArray()));
  EXPECT_TRUE(ids(all.value("polygons").toArray()).contains(cutout));
  EXPECT_EQ(2, all.value("polygons").toArray().count());  // With outline.

  const QJsonObject area = structured(
      "board_list_items",
      {{"area",
        QJsonObject{{"from", QJsonArray{0, 8}}, {"to", QJsonArray{7, 12}}}}});
  EXPECT_EQ(QStringList{via}, ids(area.value("vias").toArray()));
  EXPECT_TRUE(area.value("texts").toArray().isEmpty());
  EXPECT_TRUE(area.value("keepouts").toArray().isEmpty());

  const QJsonObject netB = structured("board_list_items", {{"net", "B"}});
  EXPECT_TRUE(netB.value("traces").toArray().isEmpty());
  EXPECT_EQ(QStringList{plane}, ids(netB.value("planes").toArray()));

  const QJsonArray pads =
      structured("board_list_items", {{"types", QJsonArray{"pads"}}})
          .value("pads")
          .toArray();
  EXPECT_EQ(4, pads.count());
  EXPECT_FALSE(structured("board_list_items", {{"types", QJsonArray{"pads"}}})
                   .contains("traces"));
}

TEST_F(DesignToolsTest, testRemoveBoardItems) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  EXPECT_EQ(2, unroutedCount());
  const QJsonArray traces =
      structured("board_route",
                 {{"from", "R1.1"}, {"to", "R2.1"}, {"layer", "top"}})
          .value("traces")
          .toArray();
  EXPECT_EQ(1, unroutedCount());
  EXPECT_EQ(traces.count(),
            structured("board_remove_items", {{"ids", traces}})
                .value("removed")
                .toInt());
  EXPECT_EQ(2, unroutedCount());

  const QString via =
      structured("board_add_via",
                 {{"net", "B"}, {"position", QJsonArray{14, 10}}})
          .value("via")
          .toString();
  ok("board_route", {{"from", "R1.2"}, {"to", via}, {"layer", "top"}});
  ok("board_route", {{"from", via}, {"to", "R2.2"}, {"layer", "top"}});
  EXPECT_EQ(1, unroutedCount());
  ok("board_remove_items", {{"ids", QJsonArray{"net:B"}}});
  EXPECT_EQ(2, unroutedCount());
  EXPECT_TRUE(structured("board_list_items").value("vias").toArray().isEmpty());
  EXPECT_TRUE(error("board_remove_items", {{"ids", QJsonArray{"via:nope"}}})
                  .contains("'via:nope'"));
}

TEST_F(DesignToolsTest, testModifyBoardItems) {
  createProjectWithTwoResistors();
  const QString via = structured("board_add_via",
                                 {{"net", "A"}, {"position", QJsonArray{5, 5}}})
                          .value("via")
                          .toString();
  ok("board_modify_item",
     {{"id", via},
      {"position", QJsonArray{6, 7}},
      {"drill", 0.4},
      {"size", 0.8}});
  const QJsonObject v =
      structured("board_list_items").value("vias").toArray().at(0).toObject();
  EXPECT_EQ(QJsonArray({6, 7}), v.value("position").toArray());
  EXPECT_DOUBLE_EQ(0.4, v.value("drill").toDouble());
  EXPECT_DOUBLE_EQ(0.8, v.value("size").toDouble());

  const QString text =
      structured("board_add_text",
                 {{"text", "Old"}, {"position", QJsonArray{1, 1}}})
          .value("text")
          .toString();
  ok("board_modify_item",
     {{"id", text}, {"text", "New"}, {"position", QJsonArray{2, 3}}});
  const QJsonObject t =
      structured("board_list_items").value("texts").toArray().at(0).toObject();
  EXPECT_EQ("New", t.value("text").toString());
  EXPECT_EQ(QJsonArray({2, 3}), t.value("position").toArray());

  const QString keepout =
      structured("board_add_keepout",
                 {{"vertices", pts({{0, 0}, {2, 0}, {2, 2}})}})
          .value("keepout")
          .toString();
  ok("board_modify_item",
     {{"id", keepout},
      {"vertices", pts({{0, 0}, {4, 0}, {4, 4}, {0, 4}})},
      {"rules", QJsonArray{"noDevices"}}});
  const BI_Zone& zone = *mSession->getBoard(0).getZones().first();
  EXPECT_EQ(5, zone.getData().getOutline().getVertices().count());
  EXPECT_EQ(Zone::Rules(Zone::Rule::NoDevices), zone.getData().getRules());

  EXPECT_TRUE(error("board_modify_item", {{"id", via}, {"text", "x"}})
                  .contains("'text'"));
}

TEST_F(DesignToolsTest, testRouteReportsClearanceViolations) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  const QJsonObject r1 =
      structured("board_place_device",
                 {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  const QJsonObject r2 =
      structured("board_place_device",
                 {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  const Point a1 = padPosition(r1, "1"), b1 = padPosition(r1, "2");
  const Point a2 = padPosition(r2, "1");
  // Across pad R1.2 of net B: a short.
  const QJsonObject bad = structured(
      "board_route",
      {{"from", "R1.1"},
       {"to", "R2.1"},
       {"layer", "top"},
       {"points",
        QJsonArray{toArray(b1 + Point::fromMm(1, 0)),
                   toArray(Point(b1.getX() + Length(1000000), a2.getY()))}}});
  EXPECT_FALSE(bad.value("violations").toArray().isEmpty())
      << QJsonDocument(bad).toJson().toStdString();
  ok("undo");
  const QJsonObject good = structured(
      "board_route",
      {{"from", "R1.1"},
       {"to", "R2.1"},
       {"layer", "top"},
       {"points",
        QJsonArray{toArray(Point(a1.getX() - Length(2000000), a1.getY())),
                   toArray(Point(a1.getX() - Length(2000000), a2.getY()))}}});
  EXPECT_TRUE(good.value("violations").toArray().isEmpty())
      << QJsonDocument(good).toJson().toStdString();
}

TEST_F(DesignToolsTest, testRouteToPointOnTrace) {
  createProjectWithTwoResistors();
  structured("schematic_add_part",
             {{"component", "Resistor"},
              {"device", "R-0805"},
              {"position", QJsonArray{80, 50}}});
  structured("schematic_connect", {{"pins", QJsonArray{"R3.1", "R1.1"}}});
  const QJsonObject r1 =
      structured("board_place_device",
                 {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  structured("board_place_device",
             {{"designator", "R3"},
              {"position", QJsonArray{3, 10}},
              {"rotation", 180}});
  const Point a1 = padPosition(r1, "1");
  // A straight trace from R1.1 upwards to the height of R2.1.
  const QJsonObject r2 = structured("board_get_part", {{"designator", "R2"}});
  const Point a2 = padPosition(r2, "1");
  ok("board_route",
     {{"from", "R1.1"},
      {"to", "R2.1"},
      {"layer", "top"},
      {"points",
       QJsonArray{QJsonValue(toArray(Point(a1.getX(), a2.getY())))}}});
  EXPECT_EQ(2, unroutedCount());  // R3.1 and net B.
  ok("board_route",
     {{"from", "R3.1"},
      {"to", toArray(Point(a1.getX(), Length(10000000)))},
      {"layer", "top"}});
  EXPECT_EQ(1, unroutedCount());  // Net B.
  EXPECT_TRUE(
      error("board_route",
            {{"from", "R2.2"}, {"to", QJsonArray{20, 20}}, {"layer", "top"}})
          .contains("No trace"));
}

TEST_F(DesignToolsTest, testRemoveTraceKeepsIdsOfOtherTraces) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  const QJsonArray traces = structured("board_route",
                                       {{"from", "R1.1"},
                                        {"to", "R2.1"},
                                        {"layer", "top"},
                                        {"points", pts({{5, 5}, {5, 15}})}})
                                .value("traces")
                                .toArray();
  ASSERT_EQ(3, traces.count());
  ok("board_remove_items", {{"ids", QJsonArray{traces.at(1)}}});
  QStringList remaining =
      ids(structured("board_list_items").value("traces").toArray());
  QStringList expected{traces.at(0).toString(), traces.at(2).toString()};
  remaining.sort();
  expected.sort();
  EXPECT_EQ(expected, remaining);
}

TEST_F(DesignToolsTest, testRouteJoiningSegmentsReturnsOnlyNewTraces) {
  createProjectWithTwoResistors();
  QStringList vias;
  for (const QJsonArray& pos : {QJsonArray{5, 8}, QJsonArray{5, 12},
                                QJsonArray{8, 8}, QJsonArray{8, 12}}) {
    vias.append(structured("board_add_via", {{"net", "A"}, {"position", pos}})
                    .value("via")
                    .toString());
  }
  const QJsonArray left =
      structured(
          "board_route",
          {{"from", vias.at(0)}, {"to", vias.at(1)}, {"layer", "bottom"}})
          .value("traces")
          .toArray();
  const QJsonArray right =
      structured(
          "board_route",
          {{"from", vias.at(2)}, {"to", vias.at(3)}, {"layer", "bottom"}})
          .value("traces")
          .toArray();
  const QJsonArray joint =
      structured(
          "board_route",
          {{"from", vias.at(1)}, {"to", vias.at(3)}, {"layer", "bottom"}})
          .value("traces")
          .toArray();
  EXPECT_EQ(1, joint.count());
  const QStringList traces =
      ids(structured("board_list_items").value("traces").toArray());
  EXPECT_TRUE(traces.contains(left.at(0).toString()));
  EXPECT_TRUE(traces.contains(right.at(0).toString()));
}

TEST_F(DesignToolsTest, testFootprintGeometryOfPlacedPart) {
  structured("library_create",
             {{"name", "Custom Parts"},
              {"dependencies", QJsonArray{"Populated Library"}}});
  auto pad = [](const QString& name, qreal x) {
    return QJsonObject{{"name", name},
                       {"position", QJsonArray{x, 0}},
                       {"size", QJsonArray{1, 1}}};
  };
  structured(
      "package_create",
      {{"library", "Custom Parts"},
       {"name", "Test Pkg"},
       {"pads", QJsonArray{pad("1", -2), pad("2", 2)}},
       {"holes",
        QJsonArray{
            QJsonObject{{"position", QJsonArray{0, 1.5}}, {"diameter", 1}}}},
       {"keepouts",
        QJsonArray{QJsonObject{{"vertices", pts({{-1, -1}, {1, -1}, {1, 1}})},
                               {"layers", "top"},
                               {"rules", QJsonArray{"noCopper"}}}}},
       {"body", QJsonArray{6, 2}}});
  const QJsonObject lib =
      structured("library_get", {{"type", "package"}, {"element", "Test Pkg"}});
  ASSERT_EQ(1, lib.value("courtyard").toArray().count());
  EXPECT_EQ(1, lib.value("keepouts").toArray().count());
  EXPECT_EQ(1, lib.value("holes").toArray().count());

  structured("device_create",
             {{"library", "Custom Parts"},
              {"name", "R Test Pkg"},
              {"component", "Resistor"},
              {"package", "Test Pkg"},
              {"padSignalMap", QJsonObject{{"1", "1"}, {"2", "2"}}}});
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  structured("schematic_add_part",
             {{"component", "Resistor"},
              {"device", "R Test Pkg"},
              {"position", QJsonArray{0, 0}}});
  const QJsonObject part = structured("board_place_device",
                                      {{"designator", "R1"},
                                       {"position", QJsonArray{10, 10}},
                                       {"rotation", 90},
                                       {"side", "bottom"}});
  const QJsonObject courtyard =
      part.value("courtyard").toArray().at(0).toObject();
  EXPECT_EQ("bottom", courtyard.value("side").toString());
  const QJsonArray vertices = courtyard.value("vertices").toArray();
  for (const QJsonValue& v : vertices) {
    // Body 6x2 and the hole up to y=2, rotated by 90°.
    EXPECT_NEAR(10, v.toArray().at(0).toDouble(), 2.3);
    EXPECT_NEAR(10, v.toArray().at(1).toDouble(), 3.6);
  }
  const QJsonObject keepout = part.value("keepouts").toArray().at(0).toObject();
  EXPECT_EQ(QJsonArray{"bottom"}, keepout.value("layers").toArray());
  EXPECT_EQ(QJsonArray{"noCopper"}, keepout.value("rules").toArray());
  const QJsonArray hole = part.value("holes")
                              .toArray()
                              .at(0)
                              .toObject()
                              .value("position")
                              .toArray();
  EXPECT_NEAR(8.5, hole.at(0).toDouble(), 0.001);  // Mirrored, then rotated.
  EXPECT_NEAR(10, hole.at(1).toDouble(), 0.001);
}

TEST_F(DesignToolsTest, testPackageWithCustomPadSlotAndLegend) {
  structured("library_create", {{"name", "Custom Parts"}});
  const QJsonObject result = structured(
      "package_create",
      {{"library", "Custom Parts"},
       {"name", "Odd Pads"},
       {"pads",
        QJsonArray{
            QJsonObject{
                {"name", "1"},
                {"position", QJsonArray{-2, 0}},
                {"size", QJsonArray{1.5, 1.5}},
                {"shape", "custom"},
                {"vertices", pts({{-0.75, -0.75}, {0.75, 0}, {-0.75, 0.75}})}},
            QJsonObject{{"name", "2"},
                        {"position", QJsonArray{2, 0}},
                        {"size", QJsonArray{3, 1.6}},
                        {"drill", 0.8},
                        {"slotLength", 2.2}},
            QJsonObject{{"name", "3"},
                        {"position", QJsonArray{0, 2}},
                        {"size", QJsonArray{1, 1}},
                        {"paste", false}}}},
       {"body", QJsonArray{6, 3}},
       {"legend", true}});
  const QJsonArray messages = result.value("messages").toArray();
  for (const QJsonValue& msg : messages) {
    const QString text = msg.toObject().value("message").toString();
    if (!text.contains("categories")) {
      EXPECT_NE("error", msg.toObject().value("severity").toString())
          << text.toStdString();
    }
  }
  const QJsonArray pads =
      structured("library_get", {{"type", "package"}, {"element", "Odd Pads"}})
          .value("footprintPads")
          .toArray();
  ASSERT_EQ(3, pads.count());
  EXPECT_EQ("custom", pads.at(0).toObject().value("shape").toString());
  EXPECT_DOUBLE_EQ(2.2, pads.at(1).toObject().value("slotLength").toDouble());

  std::unique_ptr<Package> pkg = Package::open(
      std::make_unique<TransactionalDirectory>(TransactionalFileSystem::openRO(
          mSession->getWorkspace()
              .getLocalLibrariesPath()
              .getPathTo("Custom_Parts.lplib/pkg")
              .getPathTo(result.value("uuid").toString()))));
  const Footprint& footprint = *pkg->getFootprints().first();
  int legend = 0;
  for (const Polygon& polygon : footprint.getPolygons()) {
    legend += (polygon.getLayer() == Layer::topLegend()) ? 1 : 0;
  }
  for (const Circle& circle : footprint.getCircles()) {
    legend += (circle.getLayer() == Layer::topLegend()) ? 1 : 0;
  }
  EXPECT_EQ(2, legend);  // Outline and pin 1 marker.
  EXPECT_EQ(MaskConfig::off(),
            footprint.getPads().value(2)->getSolderPasteConfig());
}

TEST_F(DesignToolsTest, testBoardSettings) {
  createProjectWithTwoResistors();
  const QJsonObject before = structured("board_get_settings");
  EXPECT_TRUE(before.value("presets").toArray().contains("default"));
  EXPECT_EQ(0, before.value("innerLayers").toInt());
  ok("board_set_settings",
     {{"preset", "default"},
      {"innerLayers", 2},
      {"thickness", 1.0},
      {"defaultTraceWidth", 0.25},
      {"defaultViaDrill", 0.35},
      {"minCopperClearance", 0.15},
      {"minCopperWidth", 0.12}});
  const QJsonObject after = structured("board_get_settings");
  EXPECT_EQ(2, after.value("innerLayers").toInt());
  EXPECT_DOUBLE_EQ(1.0, after.value("thickness").toDouble());
  const QJsonObject rules = after.value("designRules").toObject();
  EXPECT_DOUBLE_EQ(0.25, rules.value("defaultTraceWidth").toDouble());
  EXPECT_DOUBLE_EQ(0.35, rules.value("defaultViaDrill").toDouble());
  const QJsonObject drc = after.value("drc").toObject();
  EXPECT_DOUBLE_EQ(0.15, drc.value("minCopperClearance").toDouble());
  EXPECT_DOUBLE_EQ(0.12, drc.value("minCopperWidth").toDouble());
  EXPECT_TRUE(
      error("board_set_settings", {{"preset", "Nope"}}).contains("'default'"));
}

TEST_F(DesignToolsTest, testRouteOnInnerLayer) {
  createProjectWithTwoResistors();
  ok("board_set_settings", {{"innerLayers", 2}});
  const QString via1 =
      structured("board_add_via",
                 {{"net", "A"}, {"position", QJsonArray{5, 5}}})
          .value("via")
          .toString();
  const QString via2 =
      structured("board_add_via",
                 {{"net", "A"}, {"position", QJsonArray{5, 15}}})
          .value("via")
          .toString();
  ok("board_route", {{"from", via1}, {"to", via2}, {"layer", "inner2"}});
  EXPECT_EQ("inner2",
            structured("board_list_items")
                .value("traces")
                .toArray()
                .at(0)
                .toObject()
                .value("layer")
                .toString());
  ok("board_add_plane", {{"net", "B"}, {"layer", "inner1"}});
  EXPECT_TRUE(
      error("board_route", {{"from", via1}, {"to", via2}, {"layer", "inner3"}})
          .contains("'inner2'"));
}

TEST_F(DesignToolsTest, testNetClass) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  ok("board_route", {{"from", "R1.1"}, {"to", "R2.1"}, {"layer", "top"}});
  ok("net_class_set",
     {{"name", "Power"},
      {"defaultTraceWidth", 0.8},
      {"minClearance", 0.3},
      {"nets", QJsonArray{"A"}}});
  QJsonObject power;
  const QJsonArray netClasses =
      structured("board_get_settings").value("netClasses").toArray();
  for (const QJsonValue& nc : netClasses) {
    if (nc.toObject().value("name") == "Power") power = nc.toObject();
  }
  EXPECT_EQ(QJsonArray{"A"}, power.value("nets").toArray());
  EXPECT_DOUBLE_EQ(0.8, power.value("defaultTraceWidth").toDouble());
  EXPECT_DOUBLE_EQ(0.3, power.value("minClearance").toDouble());
  EXPECT_EQ((QStringList{"R1.1", "R2.1"}), netlist().value("A"));
  EXPECT_EQ(1, unroutedCount());  // The trace stays on net A.
  EXPECT_DOUBLE_EQ(
      0.5,
      structured("board_route",
                 {{"from", "R1.2"}, {"to", "R2.2"}, {"layer", "top"}})
          .value("width")
          .toDouble());  // Net B is in the default class.
  auto defaultClass = [this]() {
    const QJsonArray classes =
        structured("board_get_settings").value("netClasses").toArray();
    for (const QJsonValue& nc : classes) {
      if (nc.toObject().value("name") == "default") return nc.toObject();
    }
    return QJsonObject();
  };
  ok("net_class_set",
     {{"name", "default"}, {"defaultTraceWidth", 0.2}, {"minClearance", 0.4}});
  EXPECT_DOUBLE_EQ(0.4, defaultClass().value("minClearance").toDouble());
  ok("undo");
  EXPECT_EQ(QJsonValue(), defaultClass().value("defaultTraceWidth"));
  EXPECT_DOUBLE_EQ(0, defaultClass().value("minClearance").toDouble());
}

TEST_F(DesignToolsTest, testProjectMetadata) {
  createProjectWithTwoResistors();
  ok("project_set_metadata",
     {{"name", "Blinky"}, {"version", "v1.0"}, {"author", "Tester"}});
  const QJsonObject summary = structured("project_summary");
  EXPECT_EQ("Blinky", summary.value("name").toString());
  EXPECT_EQ("v1.0", summary.value("version").toString());
  EXPECT_EQ("Tester", summary.value("author").toString());
}

TEST_F(DesignToolsTest, testRenderRegionAndLayers) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  const QJsonObject region = call("render_board",
                                  {{"dpi", 254},
                                   {"region",
                                    QJsonObject{{"from", QJsonArray{0, 0}},
                                                {"to", QJsonArray{10, 10}}}}})
                                 .value("structuredContent")
                                 .toObject();
  EXPECT_NEAR(100, region.value("width").toInt(), 2);
  EXPECT_NEAR(100, region.value("height").toInt(), 2);
  EXPECT_NEAR(0, region.value("topLeft").toArray().at(0).toDouble(), 0.2);
  EXPECT_NEAR(10, region.value("topLeft").toArray().at(1).toDouble(), 0.2);
  ok("render_board",
     {{"dpi", 50},
      {"layers", QJsonArray{"board_outlines", "board_courtyard_top"}}});
  const QString msg = error("render_board", {{"layers", QJsonArray{"nope"}}});
  EXPECT_TRUE(msg.contains("board_copper_top")) << msg.toStdString();
  ok("render_schematic",
     {{"region",
       QJsonObject{{"from", QJsonArray{0, 0}}, {"to", QJsonArray{100, 100}}}}});
}

TEST_F(DesignToolsTest, testCustomAttributesAndPart) {
  createProjectWithTwoResistors();
  QJsonObject part = structured(
      "part_set_properties",
      {{"designator", "R1"},
       {"attributes", QJsonObject{{"tolerance", "1%"}}},
       {"part",
        QJsonObject{{"mpn", "RC0805FR-071KL"}, {"manufacturer", "Yageo"}}}});
  EXPECT_EQ("1%",
            part.value("attributes").toObject().value("TOLERANCE").toString());
  EXPECT_EQ("RC0805FR-071KL", part.value("mpn").toString());
  EXPECT_EQ("Yageo", part.value("manufacturer").toString());
  part = structured("part_set_properties",
                    {{"designator", "R1"},
                     {"attributes", QJsonObject{{"TOLERANCE", QJsonValue()}}}});
  EXPECT_FALSE(part.value("attributes").toObject().contains("TOLERANCE"));
  EXPECT_TRUE(part.value("attributes").toObject().contains("RESISTANCE"));
}

TEST_F(DesignToolsTest, testHideTextsOfSeveralParts) {
  createProjectWithTwoResistors();
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  ok("board_set_part_text",
     {{"designator", QJsonArray{"R1", "R2"}},
      {"text", "value"},
      {"visible", false}});
  for (const BI_Device* dev : mSession->getBoard(0).getDeviceInstances()) {
    for (const BI_StrokeText* text : dev->getStrokeTexts()) {
      EXPECT_NE("{{VALUE}}", text->getData().getText());
    }
  }
}

TEST_F(DesignToolsTest, testSchematicWiresTextsAndPages) {
  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  for (int x : {20, 40, 60}) {
    structured("schematic_add_part",
               {{"component", "Resistor"},
                {"device", "R-0805"},
                {"position", QJsonArray{x, 50}},
                {"rotation", 90}});
  }
  const QJsonObject wire = structured("schematic_draw_wire",
                                      {{"from", "R1.1"},
                                       {"to", "R2.1"},
                                       {"points", pts({{20, 40}, {40, 40}})}});
  const QString net = wire.value("net").toString();
  EXPECT_EQ((QStringList{"R1.1", "R2.1"}), netlist().value(net));
  structured("schematic_connect", {{"pins", QJsonArray{"R2.1"}}, {"net", "X"}});
  EXPECT_EQ(
      "X",
      structured("schematic_draw_wire", {{"from", "R3.1"}, {"to", "R1.1"}})
          .value("net")
          .toString());
  EXPECT_EQ((QStringList{"R1.1", "R2.1", "R3.1"}), netlist().value("X"));
  // From a pin with a stub of schematic_connect.
  structured("schematic_connect", {{"pins", QJsonArray{"R1.2"}}, {"net", "Y"}});
  ok("schematic_draw_wire", {{"from", "R1.2"}, {"to", "R2.2"}});
  EXPECT_EQ((QStringList{"R1.2", "R2.2"}), netlist().value("Y"));
  EXPECT_TRUE(error("schematic_draw_wire", {{"from", "R1.2"}, {"to", "R3.1"}})
                  .contains("different nets"));

  structured("schematic_add_text",
             {{"text", "Timing"}, {"position", QJsonArray{30, 70}}});
  EXPECT_EQ(1, mSession->getSchematic(0).getTexts().count());

  ok("schematic_add_page", {{"name", "Power"}});
  EXPECT_EQ((QJsonArray{"Main", "Power"}),
            structured("project_summary").value("schematics").toArray());
  structured("schematic_add_part",
             {{"component", "Resistor"},
              {"schematic", "Power"},
              {"position", QJsonArray{20, 20}}});
  EXPECT_EQ(1, mSession->getSchematic(1).getSymbols().count());
}

TEST_F(DesignToolsTest, testSchematicPartText) {
  createProjectWithTwoResistors();
  ok("schematic_set_part_text",
     {{"designator", "R1"},
      {"text", "value"},
      {"position", QJsonArray{60, 40}},
      {"rotation", 90}});
  const SI_Symbol& symbol = **mSession->getProject()
                                  .getCircuit()
                                  .getComponentInstanceByName("R1")
                                  ->getSymbols()
                                  .begin();
  bool found = false;
  for (const SI_Text* text : symbol.getTexts()) {
    if (text->getTextObj().getText() == "{{VALUE}}") {
      EXPECT_EQ(Point::fromMm(60, 40), text->getTextObj().getPosition());
      EXPECT_EQ(Angle::deg90(), text->getTextObj().getRotation());
      found = true;
    }
  }
  EXPECT_TRUE(found);
  ok("schematic_set_part_text",
     {{"designator", "R1"}, {"text", "name"}, {"visible", false}});
  for (const SI_Text* text : symbol.getTexts()) {
    EXPECT_NE("{{NAME}}", text->getTextObj().getText());
  }
}

TEST_F(DesignToolsTest, testMultiGateComponent) {
  structured("library_create", {{"name", "Custom Parts"}});
  auto pin = [](const QString& name, qreal x, qreal y, qreal rotation) {
    return QJsonObject{
        {"name", name}, {"position", QJsonArray{x, y}}, {"rotation", rotation}};
  };
  structured(
      "symbol_create",
      {{"library", "Custom Parts"},
       {"name", "Half"},
       {"pins", QJsonArray{pin("IN", -7.62, 0, 0), pin("OUT", 7.62, 0, 180)}},
       {"bodyFrom", QJsonArray{-5.08, -2.54}},
       {"bodyTo", QJsonArray{5.08, 2.54}}});
  structured(
      "symbol_create",
      {{"library", "Custom Parts"},
       {"name", "Supply"},
       {"pins", QJsonArray{pin("V+", 0, 7.62, 270), pin("V-", 0, -7.62, 90)}},
       {"bodyFrom", QJsonArray{-2.54, -5.08}},
       {"bodyTo", QJsonArray{2.54, 5.08}}});
  structured(
      "component_create",
      {{"library", "Custom Parts"},
       {"name", "Dual Buffer"},
       {"prefix", "U"},
       {"gates",
        QJsonArray{QJsonObject{{"symbol", "Half"}, {"suffix", "A"}},
                   QJsonObject{{"symbol", "Half"},
                               {"suffix", "B"},
                               {"position", QJsonArray{0, -10.16}}},
                   QJsonObject{{"symbol", "Supply"},
                               {"suffix", "P"},
                               {"position", QJsonArray{20.32, 0}}}}},
       {"signals",
        QJsonObject{{"V+", QJsonObject{{"role", "power"}, {"required", true}}},
                    {"OUT_A", QJsonObject{{"role", "output"}}}}}});
  const QJsonObject cmp = structured(
      "library_get", {{"type", "component"}, {"element", "Dual Buffer"}});
  EXPECT_EQ((QJsonArray{"IN_A", "OUT_A", "IN_B", "OUT_B", "V+", "V-"}),
            cmp.value("signals").toArray());
  EXPECT_EQ(3,
            cmp.value("symbolVariants")
                .toArray()
                .at(0)
                .toObject()
                .value("gateCount")
                .toInt());

  ok("project_create",
     {{"path", mTmpDir.getPathTo("project/test.lpp").toStr()},
      {"name", "Test"}});
  const QJsonObject part = structured(
      "schematic_add_part",
      {{"component", "Dual Buffer"}, {"position", QJsonArray{50, 50}}});
  ASSERT_EQ(3, part.value("symbols").toArray().count());
  EXPECT_TRUE(error("schematic_move_part",
                    {{"designator", "U1"}, {"position", QJsonArray{0, 0}}})
                  .contains("gate"));
  const QJsonObject moved = structured("schematic_move_part",
                                       {{"designator", "U1"},
                                        {"gate", "B"},
                                        {"position", QJsonArray{100, 100}}});
  int atNewPosition = 0;
  const QJsonArray symbols = moved.value("symbols").toArray();
  for (const QJsonValue& symbol : symbols) {
    if (symbol.toObject().value("position") == QJsonArray({100, 100})) {
      EXPECT_EQ("B", symbol.toObject().value("gate").toString());
      ++atNewPosition;
    }
  }
  EXPECT_EQ(1, atNewPosition);
  // The required power pin is not connected.
  bool required = false;
  const QJsonArray messages = structured("erc_run").value("messages").toArray();
  for (const QJsonValue& msg : messages) {
    required |= msg.toObject().value("message").toString().contains("V+");
  }
  EXPECT_TRUE(required);
}

TEST_F(DesignToolsTest, testQueryBoardArea) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  const QJsonObject r1 =
      structured("board_place_device",
                 {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  const QJsonObject r2 =
      structured("board_place_device",
                 {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  const Point a1 = padPosition(r1, "1"), a2 = padPosition(r2, "1");
  structured("board_route",
             {{"from", "R1.1"},
              {"to", "R2.1"},
              {"layer", "top"},
              {"points",
               QJsonArray{QJsonArray{5, a1.getY().toMm()},
                          QJsonArray{5, a2.getY().toMm()}}}});
  const QString via =
      structured("board_add_via",
                 {{"net", "B"}, {"position", QJsonArray{20, 10}}})
          .value("via")
          .toString();
  auto query = [this](qreal x1, qreal y1, qreal x2, qreal y2) {
    return structured(
        "board_query",
        {{"from", QJsonArray{x1, y1}}, {"to", QJsonArray{x2, y2}}});
  };

  // The route passes through the area: the whole path in order.
  QJsonObject result = query(3, 8, 7, 12);
  EXPECT_TRUE(result.value("parts").toArray().isEmpty());
  QJsonArray paths = result.value("paths").toArray();
  ASSERT_EQ(1, paths.count());
  const QJsonObject path = paths.at(0).toObject();
  EXPECT_EQ("A", path.value("net").toString());
  EXPECT_EQ("top", path.value("layer").toString());
  EXPECT_EQ(3, path.value("traces").toArray().count());
  EXPECT_EQ(4, path.value("points").toArray().count());
  EXPECT_EQ(
      (QSet<QString>{"R1.1", "R2.1"}),
      (QSet<QString>{path.value("from").toObject().value("pad").toString(),
                     path.value("to").toObject().value("pad").toString()}));
  EXPECT_EQ(QJsonObject({{"top", false}, {"bottom", true}}),
            result.value("free").toObject());
  EXPECT_TRUE(result.value("withinBoard").toBool());
  EXPECT_EQ(
      QJsonObject({{"top", true}, {"bottom", true}}),
      structured(
          "board_query",
          {{"from", QJsonArray{3, 8}}, {"to", QJsonArray{7, 12}}, {"net", "A"}})
          .value("free")
          .toObject());

  // A part with its pads in the area.
  result = query(8, 3, 12, 7);
  const QJsonArray parts = result.value("parts").toArray();
  ASSERT_EQ(1, parts.count());
  EXPECT_EQ("R1", parts.at(0).toObject().value("designator").toString());
  EXPECT_EQ(2, parts.at(0).toObject().value("pads").toArray().count());
  EXPECT_TRUE(parts.at(0).toObject().contains("box"));
  const QJsonArray texts = result.value("texts").toArray();
  for (const QJsonValue& text : texts) {
    EXPECT_TRUE(text.toObject().contains("box"));
  }

  // Empty space and the board edge.
  result = query(20, 2, 24, 4);
  EXPECT_TRUE(result.value("parts").toArray().isEmpty());
  EXPECT_TRUE(result.value("paths").toArray().isEmpty());
  EXPECT_EQ(QJsonObject({{"top", true}, {"bottom", true}}),
            result.value("free").toObject());
  EXPECT_FALSE(query(28, 8, 32, 12).value("withinBoard").toBool());

  // A via blocks both layers.
  result = query(19, 9, 21, 11);
  EXPECT_EQ(QStringList{via}, ids(result.value("vias").toArray()));
  EXPECT_EQ(QJsonObject({{"top", false}, {"bottom", false}}),
            result.value("free").toObject());
}

TEST_F(DesignToolsTest, testRelativePathsAreRejected) {
  const QString dir = "mcp-relative-path-test";
  EXPECT_TRUE(error("workspace_open", {{"path", dir}}).contains("absolute"));
  EXPECT_TRUE(error("project_create", {{"path", dir + "/p.lpp"}, {"name", "P"}})
                  .contains("absolute"));
  EXPECT_TRUE(
      error("project_open", {{"path", dir + "/p.lpp"}}).contains("absolute"));
  EXPECT_FALSE(QDir(dir).exists());
  QDir(dir).removeRecursively();
}

TEST_F(DesignToolsTest, testPartTextIdsAddressTheirPart) {
  createProjectWithTwoResistors();
  ok("board_set_outline", {{"width", 30}, {"height", 20}});
  structured("board_place_device",
             {{"designator", "R1"}, {"position", QJsonArray{10, 5}}});
  structured("board_place_device",
             {{"designator", "R2"}, {"position", QJsonArray{10, 15}}});
  auto nameTexts = [this]() {
    QMap<QString, QJsonObject> texts;
    const QJsonArray textItems =
        structured("board_list_items", {{"types", QJsonArray{"texts"}}})
            .value("texts")
            .toArray();
    for (const QJsonValue& value : textItems) {
      const QJsonObject text = value.toObject();
      if (text.value("text") == "{{NAME}}") {
        texts.insert(text.value("part").toString(), text);
      }
    }
    return texts;
  };
  const QMap<QString, QJsonObject> before = nameTexts();
  ASSERT_EQ((QStringList{"R1", "R2"}), before.keys());
  const QString id = before.value("R2").value("id").toString();
  EXPECT_NE(before.value("R1").value("id").toString(), id);

  ok("board_modify_item", {{"id", id}, {"height", 2.5}});
  const QMap<QString, QJsonObject> after = nameTexts();
  EXPECT_EQ(2.5, after.value("R2").value("height").toDouble());
  EXPECT_EQ(before.value("R1").value("height"),
            after.value("R1").value("height"));

  const QString uuid = id.mid(id.lastIndexOf(':') + 1);
  const QString msg =
      error("board_modify_item", {{"id", "text:" + uuid}, {"height", 2}});
  EXPECT_TRUE(msg.contains(id)) << msg.toStdString();
}

TEST_F(DesignToolsTest, testSupplySymbolNamesTheNetOfAWire) {
  createProjectWithTwoResistors();
  for (int i : {3, 4}) {
    structured("schematic_add_part",
               {{"component", "Resistor"},
                {"device", "R-0805"},
                {"designator", QString("R%1").arg(i)},
                {"position", QJsonArray{20.32 * i, 50.8}}});
    structured("schematic_add_part",
               {{"component", "Supply GND"},
                {"designator", QString("GND%1").arg(i)},
                {"position", QJsonArray{20.32 * i, 25.4}}});
  }
  structured("schematic_connect", {{"pins", QJsonArray{"GND3.Net"}}});
  const QString autoNet =
      structured("schematic_draw_wire", {{"from", "R3.1"}, {"to", "R4.1"}})
          .value("net")
          .toString();
  EXPECT_TRUE(autoNet.startsWith("N")) << autoNet.toStdString();
  EXPECT_EQ(
      "GND",
      structured("schematic_draw_wire", {{"from", "R4.1"}, {"to", "GND4.Net"}})
          .value("net")
          .toString());
  const QMap<QString, QStringList> nets = netlist();
  EXPECT_EQ((QStringList{"GND3.Net", "GND4.Net", "R3.1", "R4.1"}),
            nets.value("GND"));
  EXPECT_EQ(QStringList(),
            QStringList(nets.keyBegin(), nets.keyEnd())
                .filter(QRegularExpression("^N\\d")));
  structured("schematic_connect",
             {{"pins", QJsonArray{"R3.2", "R4.2"}}, {"net", "B"}});
  EXPECT_EQ(0, structured("erc_run").value("messages").toArray().count());
}

TEST_F(DesignToolsTest, testRenamingAnAutoNamedNetToAnExistingNetMergesIt) {
  createProjectWithTwoResistors();
  structured("schematic_add_part",
             {{"component", "Resistor"},
              {"device", "R-0805"},
              {"designator", "R3"},
              {"position", QJsonArray{100, 50}}});
  const QString autoNet =
      structured("schematic_connect", {{"pins", QJsonArray{"R3.1"}}})
          .value("net")
          .toString();
  ok("net_rename", {{"net", autoNet}, {"name", "A"}});
  EXPECT_EQ((QStringList{"R1.1", "R2.1", "R3.1"}), netlist().value("A"));
  EXPECT_TRUE(
      error("net_rename", {{"net", "A"}, {"name", "B"}}).contains("exists"));
}

TEST_F(DesignToolsTest, testOutputJobsAreSavedWithTheProject) {
  createProjectWithTwoResistors();
  ok("project_save");
  structured("output_jobs_run");
  EXPECT_TRUE(structured("project_summary").value("unsavedChanges").toBool());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace automation
}  // namespace librepcb
