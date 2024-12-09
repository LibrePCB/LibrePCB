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
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectjsonexport.h>
#include <librepcb/core/types/pcbcolor.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ProjectJsonExportTest : public ::testing::Test {
protected:
  std::shared_ptr<AssemblyVariant> createAssemblyVariant() const {
    return std::make_shared<AssemblyVariant>(
        Uuid::fromString("bb0d66f1-2f21-4592-b923-d853867a6124"),
        FileProofName("AV0"), "Hello World!");
  }
  std::unique_ptr<Board> createBoard(Project& project) const {
    std::unique_ptr<Board> board(new Board(
        project,
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory()),
        "board", Uuid::fromString("1ff89be5-dd83-4b08-8d95-d09e0fd72b25"),
        ElementName("New Board")));
    board->setInnerLayerCount(5);
    board->setPcbThickness(PositiveLength(Length(1500000)));
    board->setSolderResist(&PcbColor::black());
    board->setSilkscreenColor(PcbColor::blue());
    board->setSilkscreenLayersTop({&Layer::topLegend()});
    board->setSilkscreenLayersBot({});
    return board;
  }
  std::unique_ptr<Project> createProject() const {
    std::unique_ptr<Project> project = Project::create(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
            TransactionalFileSystem::openRW(FilePath::getRandomTempPath()))),
        "project.lpp");
    project->setUuid(Uuid::fromString("7b3985b2-91ad-4e93-8d15-7668869ed45d"));
    project->setName(ElementName("New Project"));
    project->setAuthor("New Author");
    project->setVersion(FileProofName("New-Version.1"));
    project->setCreated(QDateTime(QDate(2000, 1, 2), QTime(1, 2, 3), Qt::UTC));
    project->getCircuit().addAssemblyVariant(createAssemblyVariant());
    while (project->getCircuit().getAssemblyVariants().count() > 1) {
      project->getCircuit().removeAssemblyVariant(
          project->getCircuit().getAssemblyVariants().first());
    }
    std::unique_ptr<Board> board = createBoard(*project);
    project->addBoard(*board.release());
    return project;
  }
  std::string fmt(const QByteArray& json) const {
    return QJsonDocument::fromJson(json).toJson().toStdString();
  }
  std::string fmt(const QJsonObject& json) const {
    return QJsonDocument(json).toJson().toStdString();
  }
  std::string fmt(const QJsonArray& json) const {
    return QJsonDocument(json).toJson().toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ProjectJsonExportTest, testStringList) {
  ProjectJsonExport exp;
  EXPECT_EQ(fmt("[]"), fmt(exp.toJson(QStringList{})));
  EXPECT_EQ(fmt("[\"foo\"]"), fmt(exp.toJson(QStringList{"foo"})));
  EXPECT_EQ(fmt("[\"foo\", \"bar\"]"),
            fmt(exp.toJson(QStringList{"foo", "bar"})));
}

TEST_F(ProjectJsonExportTest, testLength) {
  ProjectJsonExport exp;
  EXPECT_EQ(QJsonValue(qreal(-5.5)), exp.toJson(Length(-5500000)));
}

TEST_F(ProjectJsonExportTest, testOptionalLength) {
  ProjectJsonExport exp;
  EXPECT_TRUE(exp.toJson(std::optional<Length>()).isNull());
  EXPECT_EQ(QJsonValue(qreal(-5.5)),
            exp.toJson(std::make_optional(Length(-5500000))));
}

TEST_F(ProjectJsonExportTest, testLengthSet) {
  ProjectJsonExport exp;
  EXPECT_EQ(fmt("[]"), fmt(exp.toJson(QSet<Length>{})));
  EXPECT_EQ(fmt("[0.1]"), fmt(exp.toJson(QSet<Length>{Length(100000)})));
  EXPECT_EQ(fmt("[-0.1, 0.1]"),
            fmt(exp.toJson(QSet<Length>{Length(100000), Length(-100000)})));
}

TEST_F(ProjectJsonExportTest, testPcbColor) {
  ProjectJsonExport exp;
  EXPECT_EQ(QJsonValue("none"),
            exp.toJson(static_cast<const PcbColor*>(nullptr)));
  EXPECT_EQ(QJsonValue("black"), exp.toJson(&PcbColor::black()));
}

TEST_F(ProjectJsonExportTest, testAssemblyVariant) {
  std::shared_ptr<AssemblyVariant> av = createAssemblyVariant();

  ProjectJsonExport exp;
  const char* expected =
      "{"
      " \"uuid\": \"bb0d66f1-2f21-4592-b923-d853867a6124\","
      " \"name\": \"AV0\","
      " \"description\": \"Hello World!\""
      "}";
  EXPECT_EQ(fmt(expected), fmt(exp.toJson(*av)));
}

TEST_F(ProjectJsonExportTest, testBoundingBox) {
  auto makeBox = [](LengthBase_t x0, LengthBase_t y0, LengthBase_t x1,
                    LengthBase_t y1) {
    return ProjectJsonExport::BoundingBox{std::make_pair(
        Point(Length(x0), Length(y0)), Point(Length(x1), Length(y1)))};
  };

  ProjectJsonExport exp;
  EXPECT_EQ(QJsonValue(QJsonValue::Type::Null),
            exp.toJson(ProjectJsonExport::BoundingBox{std::nullopt}));
  const char* expected =
      "{"
      " \"x\": 0,"
      " \"y\": 0,"
      " \"width\": 0,"
      " \"height\": 0"
      "}";
  EXPECT_EQ(fmt(expected), fmt(exp.toJson(makeBox(0, 0, 0, 0)).toObject()));
  expected =
      "{"
      " \"x\": -1.1,"
      " \"y\": 2.2,"
      " \"width\": 5.5,"
      " \"height\": 6.6"
      "}";
  EXPECT_EQ(
      fmt(expected),
      fmt(exp.toJson(makeBox(-1100000, 2200000, 4400000, 8800000)).toObject()));
  expected =
      "{"
      " \"x\": -1.1,"
      " \"y\": 2.2,"
      " \"width\": 5.5,"
      " \"height\": 6.6"
      "}";
  EXPECT_EQ(
      fmt(expected),
      fmt(exp.toJson(makeBox(4400000, 8800000, -1100000, 2200000)).toObject()));
  expected =
      "{"
      " \"x\": -1.1,"
      " \"y\": 2.2,"
      " \"width\": 5.5,"
      " \"height\": 6.6"
      "}";
  EXPECT_EQ(
      fmt(expected),
      fmt(exp.toJson(makeBox(-1100000, 8800000, 4400000, 2200000)).toObject()));
}

TEST_F(ProjectJsonExportTest, testBoard) {
  std::unique_ptr<Project> project = createProject();
  std::unique_ptr<Board> board = createBoard(*project);
  board->addPolygon(*new BI_Polygon(
      *board,
      BoardPolygonData(Uuid::createRandom(), Layer::boardOutlines(),
                       UnsignedLength(0),
                       Path({Vertex(Point(Length(5000000), Length(6000000))),
                             Vertex(Point(Length(5000000), Length(10000000))),
                             Vertex(Point(Length(7000000), Length(6000000))),
                             Vertex(Point(Length(5000000), Length(6000000)))}),
                       false, false, false)));

  ProjectJsonExport exp;
  const char* expected =
      "{"
      " \"uuid\": \"1ff89be5-dd83-4b08-8d95-d09e0fd72b25\","
      " \"name\": \"New Board\","
      " \"directory\": \"board\","
      " \"inner_layers\": 5,"
      " \"pcb_thickness\": 1.5,"
      " \"solder_resist\": \"black\","
      " \"silkscreen_top\": \"blue\","
      " \"silkscreen_bottom\": \"none\","
      " \"bounding_box\": {"
      "  \"x\": 5.0,"
      "  \"y\": 6.0,"
      "  \"width\": 2.0,"
      "  \"height\": 4.0"
      " },"
      " \"vias_tht\": {\"count\": 0, \"diameters\": []},"
      " \"vias_blind\": {\"count\": 0, \"diameters\": []},"
      " \"vias_buried\": {\"count\": 0, \"diameters\": []},"
      " \"pth_drills\": {\"count\": 0, \"diameters\": []},"
      " \"pth_slots\": {\"count\": 0, \"diameters\": []},"
      " \"npth_drills\": {\"count\": 0, \"diameters\": []},"
      " \"npth_slots\": {\"count\": 0, \"diameters\": []},"
      " \"min_copper_width\": null"
      "}";
  EXPECT_EQ(fmt(expected), fmt(exp.toJson(*board)));
}

TEST_F(ProjectJsonExportTest, testProject) {
  std::unique_ptr<Project> project = createProject();

  ProjectJsonExport exp;
  const char* expected =
      "{"
      " \"filename\": \"project.lpp\","
      " \"uuid\": \"7b3985b2-91ad-4e93-8d15-7668869ed45d\","
      " \"name\": \"New Project\","
      " \"author\": \"New Author\","
      " \"version\": \"New-Version.1\","
      " \"created\": \"2000-01-02T01:02:03Z\","
      " \"locales\": [],"
      " \"norms\": [],"
      " \"variants\": ["
      "  {"
      "   \"uuid\": \"bb0d66f1-2f21-4592-b923-d853867a6124\","
      "   \"name\": \"AV0\","
      "   \"description\": \"Hello World!\""
      "  }"
      " ],"
      " \"boards\": ["
      "  {"
      "   \"uuid\": \"1ff89be5-dd83-4b08-8d95-d09e0fd72b25\","
      "   \"name\": \"New Board\","
      "   \"directory\": \"board\","
      "   \"inner_layers\": 5,"
      "   \"pcb_thickness\": 1.5,"
      "   \"solder_resist\": \"black\","
      "   \"silkscreen_top\": \"blue\","
      "   \"silkscreen_bottom\": \"none\","
      "   \"bounding_box\": null,"
      "   \"vias_tht\": {\"count\": 0, \"diameters\": []},"
      "   \"vias_blind\": {\"count\": 0, \"diameters\": []},"
      "   \"vias_buried\": {\"count\": 0, \"diameters\": []},"
      "   \"pth_drills\": {\"count\": 0, \"diameters\": []},"
      "   \"pth_slots\": {\"count\": 0, \"diameters\": []},"
      "   \"npth_drills\": {\"count\": 0, \"diameters\": []},"
      "   \"npth_slots\": {\"count\": 0, \"diameters\": []},"
      "   \"min_copper_width\": null"
      "  }"
      " ]"
      "}";
  EXPECT_EQ(fmt(expected), fmt(exp.toJson(*project)));
}

TEST_F(ProjectJsonExportTest, testProjectToUtf8) {
  std::unique_ptr<Project> project = createProject();

  ProjectJsonExport exp;
  const char* expected =
      "{"
      " \"format\": {"
      "  \"major\": 1,"  // Do not modify for LibrePCB v1.x!!!
      "  \"minor\": 0,"  // Increment on every minor change!
      "  \"type\": \"librepcb-project\""
      " },"
      " \"project\": {"
      "  \"filename\": \"project.lpp\","
      "  \"uuid\": \"7b3985b2-91ad-4e93-8d15-7668869ed45d\","
      "  \"name\": \"New Project\","
      "  \"author\": \"New Author\","
      "  \"version\": \"New-Version.1\","
      "  \"created\": \"2000-01-02T01:02:03Z\","
      "  \"locales\": [],"
      "  \"norms\": [],"
      "  \"variants\": ["
      "   {"
      "    \"uuid\": \"bb0d66f1-2f21-4592-b923-d853867a6124\","
      "    \"name\": \"AV0\","
      "    \"description\": \"Hello World!\""
      "   }"
      "  ],"
      "  \"boards\": ["
      "   {"
      "    \"uuid\": \"1ff89be5-dd83-4b08-8d95-d09e0fd72b25\","
      "    \"name\": \"New Board\","
      "    \"directory\": \"board\","
      "    \"inner_layers\": 5,"
      "    \"pcb_thickness\": 1.5,"
      "    \"solder_resist\": \"black\","
      "    \"silkscreen_top\": \"blue\","
      "    \"silkscreen_bottom\": \"none\","
      "    \"bounding_box\": null,"
      "    \"vias_tht\": {\"count\": 0, \"diameters\": []},"
      "    \"vias_blind\": {\"count\": 0, \"diameters\": []},"
      "    \"vias_buried\": {\"count\": 0, \"diameters\": []},"
      "    \"pth_drills\": {\"count\": 0, \"diameters\": []},"
      "    \"pth_slots\": {\"count\": 0, \"diameters\": []},"
      "    \"npth_drills\": {\"count\": 0, \"diameters\": []},"
      "    \"npth_slots\": {\"count\": 0, \"diameters\": []},"
      "    \"min_copper_width\": null"
      "   }"
      "  ]"
      " }"
      "}";
  EXPECT_EQ(fmt(expected), fmt(exp.toUtf8(*project)));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
