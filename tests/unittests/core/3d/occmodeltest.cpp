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
#include <librepcb/core/3d/occmodel.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/geometry/path.h>
#include <librepcb/core/utils/transform.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class OccModelTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(OccModelTest, testCreateAssembly) {
  if (OccModel::isAvailable()) {
    std::unique_ptr<OccModel> model = OccModel::createAssembly("Test Assembly");
  } else {
    EXPECT_THROW(OccModel::createAssembly("Test Assembly"), Exception);
  }
}

TEST_F(OccModelTest, testCreateBoardWithoutHoles) {
  const Path outline = Path::obround(Point(0, 0), Point(50000000, 50000000),
                                     PositiveLength(10000000));
  const QVector<Path> holes;
  if (OccModel::isAvailable()) {
    std::unique_ptr<OccModel> model = OccModel::createBoard(
        outline, holes, PositiveLength(1000000), Qt::black);
  } else {
    EXPECT_THROW(OccModel::createBoard(outline, holes, PositiveLength(1000000),
                                       Qt::black),
                 Exception);
  }
}

TEST_F(OccModelTest, testCreateBoardWithHoles) {
  const Path outline = Path::obround(Point(0, 0), Point(50000000, 50000000),
                                     PositiveLength(10000000));
  const QVector<Path> holes{
      Path::circle(PositiveLength(1000000)),
      Path::rect(Point(5000000, 5000000), Point(10000000, 10000000)),
  };
  if (OccModel::isAvailable()) {
    std::unique_ptr<OccModel> model = OccModel::createBoard(
        outline, holes, PositiveLength(1000000), Qt::black);
  } else {
    EXPECT_THROW(OccModel::createBoard(outline, holes, PositiveLength(1000000),
                                       Qt::black),
                 Exception);
  }
}

// The polygon in this test is a real use-case which failed to export due to
// numeric inaccuracies of arcs.
TEST_F(OccModelTest, testCreateBoardComplexOutline) {
  if (!OccModel::isAvailable()) {
    GTEST_SKIP();
  }

  const Path outline({
      Vertex(Point(43025447, 19304541), Angle::deg0()),
      Vertex(Point(56542699, 19304541), Angle(50675000)),
      Vertex(Point(60472409, 21165259), Angle::deg0()),
      Vertex(Point(98944660, 68127536), Angle(-13616000)),
      Vertex(Point(99812946, 68962151), Angle::deg0()),
      Vertex(Point(109762866, 76476139), Angle(90000000)),
      Vertex(Point(110755338, 83591460), Angle::deg0()),
      Vertex(Point(106049435, 89822952), Angle(40708000)),
      Vertex(Point(103071931, 91726180), Angle::deg0()),
      Vertex(Point(95945914, 93271181), Angle(-40708000)),
      Vertex(Point(92968409, 95174409), Angle::deg0()),
      Vertex(Point(82946383, 108445439), Angle(90000000)),
      Vertex(Point(75831062, 109437911), Angle::deg0()),
      Vertex(Point(64468733, 100857299), Angle(90000000)),
      Vertex(Point(63476261, 93741978), Angle::deg0()),
      Vertex(Point(74733128, 78835789), Angle(-49334000)),
      Vertex(Point(75643100, 74694361), Angle::deg0()),
      Vertex(Point(73426203, 64505146), Angle(-21336000)),
      Vertex(Point(72693045, 62773124), Angle::deg0()),
      Vertex(Point(61201151, 45483496), Angle(-44700000)),
      Vertex(Point(57199238, 42780209), Angle::deg0()),
      Vertex(Point(41507850, 39533900), Angle(-52475000)),
      Vertex(Point(36073271, 40944336), Angle::deg0()),
      Vertex(Point(33945392, 42780209), Angle(-67380000)),
      Vertex(Point(32060026, 49567925), Angle(256860000)),
      Vertex(Point(17127085, 44055011), Angle(-149836000)),
      Vertex(Point(10138072, 20037785), Angle(215794000)),
      Vertex(Point(15712129, 1521862), Angle::deg0()),
      Vertex(Point(40137093, 18403513), Angle(-34651000)),
      Vertex(Point(43025447, 19304541), Angle::deg0()),
  });
  std::unique_ptr<OccModel> model = OccModel::createBoard(
      outline, QVector<Path>(), PositiveLength(1000000), Qt::black);
}

TEST_F(OccModelTest, testLoadStepValid) {
  const FilePath fp(TEST_DATA_DIR
                    "/unittests/librepcbcommon/OccModelTest/model.step");
  const QByteArray content = FileUtils::readFile(fp);

  if (OccModel::isAvailable()) {
    std::unique_ptr<OccModel> model = OccModel::loadStep(content);
  } else {
    EXPECT_THROW(OccModel::loadStep(content), Exception);
  }
}

TEST_F(OccModelTest, testLoadStepInvalid) {
  EXPECT_THROW(OccModel::loadStep(QByteArray()), Exception);
}

TEST_F(OccModelTest, testBuildAndSaveAssembly) {
  if (!OccModel::isAvailable()) {
    GTEST_SKIP();
  }

  const Path outline = Path::obround(Point(0, 0), Point(50000000, 50000000),
                                     PositiveLength(10000000));
  const QVector<Path> holes{
      Path::circle(PositiveLength(1000000)),
      Path::rect(Point(5000000, 5000000), Point(10000000, 10000000)),
  };
  const FilePath modelFp(TEST_DATA_DIR
                         "/unittests/librepcbcommon/OccModelTest/model.step");
  const QByteArray content = FileUtils::readFile(modelFp);

  std::unique_ptr<OccModel> assembly =
      OccModel::createAssembly("Test Assembly");

  std::unique_ptr<OccModel> pcb =
      OccModel::createBoard(outline, holes, PositiveLength(1000000), Qt::black);
  assembly->addToAssembly(
      *pcb, std::make_tuple(Length(0), Length(-1000), Length(2000)),
      std::make_tuple(Angle::deg0(), -Angle::deg90(), Angle::deg180()),
      Transform(Point(Length(10000), Length(20000)), Angle::deg45(), false),
      "PCB");

  std::unique_ptr<OccModel> step1 = OccModel::loadStep(content);
  assembly->addToAssembly(
      *step1, std::make_tuple(Length(0), Length(-1000), Length(2000)),
      std::make_tuple(Angle::deg0(), -Angle::deg90(), Angle::deg180()),
      Transform(Point(Length(10000), Length(20000)), Angle::deg45(), false),
      "X1");

  std::unique_ptr<OccModel> step2 = OccModel::loadStep(content);
  assembly->addToAssembly(
      *step2, std::make_tuple(Length(0), Length(-1000), Length(2000)),
      std::make_tuple(Angle::deg0(), -Angle::deg90(), Angle::deg180()),
      Transform(Point(Length(10000), Length(20000)), Angle::deg45(), true),
      "X2");

  const FilePath outFp = FilePath::getRandomTempPath().getPathTo("te st.step");
  assembly->saveAsStep("PCB Assembly", outFp);

  // Read back.
  const QByteArray outContent = FileUtils::readFile(outFp);
  std::unique_ptr<OccModel> outModel = OccModel::loadStep(outContent);
}

TEST_F(OccModelTest, testTesselate) {
  if (OccModel::isAvailable()) {
    const FilePath fp(TEST_DATA_DIR
                      "/unittests/librepcbcommon/OccModelTest/model.step");
    const QByteArray content = FileUtils::readFile(fp);
    std::unique_ptr<OccModel> model = OccModel::loadStep(content);
    const QMap<OccModel::Color, QVector<QVector3D>> result = model->tesselate();
    for (auto points : result) {
      EXPECT_GE(points.count(), 3);
      EXPECT_EQ(points.count() % 3, 0);
    }
    EXPECT_GE(result.count(), 1);
  } else {
    GTEST_SKIP();
  }
}

TEST_F(OccModelTest, testMinifyStep) {
  const QByteArray input =
      "header;\n"
      "DATA;\n"
      "#1 = 42;\n"
      "#2 = 42;\n"
      "#3 = FOO(#1, #2);\n"
      "#4 = FOO(#2, #1);\n"
      "#5 = PRODUCT_DEFINITION(#2, #3);\n"
      "#6 = PRODUCT_DEFINITION(#2, #3);\n"
      "#7 = SHAPE_REPRESENTATION(#2, #3);\n"
      "#8 = SHAPE_REPRESENTATION(#2, #3);\n"
      "#9 = ANYREPRESENTATION(#2, #3);\n"
      "#10 = ANYREPRESENTATION(#2, #3);\n"
      "ENDSEC;\n"
      "footer;\n";
  const QByteArray expected =
      "header;\n"
      "DATA;\n"
      "#1=42;\n"
      "#2=FOO(#1, #1);\n"
      "#3=PRODUCT_DEFINITION(#1, #2);\n"  // Merging not allowed!
      "#4=PRODUCT_DEFINITION(#1, #2);\n"  // Merging not allowed!
      "#5=SHAPE_REPRESENTATION(#1, #2);\n"  // Merging not allowed!
      "#6=SHAPE_REPRESENTATION(#1, #2);\n"  // Merging not allowed!
      "#7=ANYREPRESENTATION(#1, #2);\n"  // Merging not allowed!
      "#8=ANYREPRESENTATION(#1, #2);\n"  // Merging not allowed!
      "ENDSEC;\n"
      "footer;\n";
  const QByteArray result = OccModel::minifyStep(input);
  EXPECT_EQ(expected.toStdString(), result.toStdString());
}

TEST_F(OccModelTest, testMinifyStepValid) {
  const FilePath fp(TEST_DATA_DIR
                    "/unittests/librepcbcommon/OccModelTest/model.step");
  const QByteArray content = FileUtils::readFile(fp);
  const QByteArray result = OccModel::minifyStep(content);
  EXPECT_LE(result.count(), content.count());
  EXPECT_GT(result.count(), 0);

  // Validate minified STEP file.
  if (OccModel::isAvailable()) {
    EXPECT_NO_THROW(OccModel::loadStep(result));
  }

  // Check that additional minification has no effect.
  const QByteArray result2 = OccModel::minifyStep(result);
  EXPECT_EQ(result, result2);
}

// https://github.com/LibrePCB/LibrePCB/issues/1286
TEST_F(OccModelTest, testMinifyStepColors) {
  const FilePath fp(TEST_DATA_DIR
                    "/unittests/librepcbcommon/OccModelTest/colors.step");
  const QByteArray content = FileUtils::readFile(fp);
  const QByteArray result = OccModel::minifyStep(content);
  EXPECT_LE(result.count(), content.count());
  EXPECT_GT(result.count(), 0);

  // Validate minified STEP file.
  if (OccModel::isAvailable()) {
    std::unique_ptr<OccModel> model = OccModel::loadStep(result);
    QMap<OccModel::Color, QVector<QVector3D>> triangles = model->tesselate();
    EXPECT_EQ(5, triangles.count());
  }

  // Check that additional minification has no effect.
  const QByteArray result2 = OccModel::minifyStep(result);
  EXPECT_EQ(result, result2);
}

TEST_F(OccModelTest, testMinifyStepInvalid) {
  EXPECT_THROW(OccModel::minifyStep(QByteArray()), Exception);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
