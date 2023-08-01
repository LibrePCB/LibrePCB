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

TEST_F(OccModelTest, testMinifyStepInvalid) {
  EXPECT_THROW(OccModel::minifyStep(QByteArray()), Exception);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
