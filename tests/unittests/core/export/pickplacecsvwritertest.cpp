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
#include <librepcb/common/fileio/csvfile.h>
#include <librepcb/common/pnp/pickplacecsvwriter.h>
#include <librepcb/common/pnp/pickplacedata.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class PickPlaceCsvWriterTest : public ::testing::Test {
protected:
  std::shared_ptr<PickPlaceData> createData() const noexcept {
    std::shared_ptr<PickPlaceData> data =
        std::make_shared<PickPlaceData>("project", "version", "board");
    data->addItem(PickPlaceDataItem("R10", "", "device", "pack,\"age\"",
                                    Point(-1000000, -2000000), -Angle::deg45(),
                                    PickPlaceDataItem::BoardSide::TOP));
    data->addItem(PickPlaceDataItem("U5", "1kΩ\r\n\r\n", "device", "package",
                                    Point(1000000, 2000000), Angle::deg45(),
                                    PickPlaceDataItem::BoardSide::BOTTOM));
    data->addItem(PickPlaceDataItem("R1", " 1kΩ\n1W\n100V ", "device \"foo\"",
                                    "pack,age", Point(1000000, 2000000),
                                    Angle::deg45(),
                                    PickPlaceDataItem::BoardSide::TOP));
    return data;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(PickPlaceCsvWriterTest, testEmptyData) {
  PickPlaceData data("project", "version", "board");
  PickPlaceCsvWriter writer(data);
  writer.setIncludeMetadataComment(false);
  std::shared_ptr<CsvFile> file = writer.generateCsv();
  EXPECT_EQ(
      "Designator,Value,Device,Package,Position X,Position Y,Rotation,Side\n",
      file->toString().toStdString());
}

TEST_F(PickPlaceCsvWriterTest, testBothSides) {
  std::shared_ptr<PickPlaceData> data = createData();
  PickPlaceCsvWriter writer(*data);
  std::shared_ptr<CsvFile> file = writer.generateCsv();
  QStringList lines = file->toString().split("\n");
  EXPECT_EQ("# Pick&Place Position Data File", lines[0].toStdString());
  EXPECT_EQ("#", lines[1].toStdString());
  EXPECT_EQ("# Project Name:        project", lines[2].toStdString());
  EXPECT_EQ("# Project Version:     version", lines[3].toStdString());
  EXPECT_EQ("# Board Name:          board", lines[4].toStdString());
  EXPECT_EQ("# Unit:                mm", lines[7].toStdString());
  EXPECT_EQ("# Rotation:            Degrees CCW", lines[8].toStdString());
  EXPECT_EQ("# Board Side:          Top + Bottom", lines[9].toStdString());
  EXPECT_EQ("", lines[10].toStdString());
  EXPECT_EQ(
      "Designator,Value,Device,Package,Position X,Position Y,Rotation,Side",
      lines[11].toStdString());
  EXPECT_EQ(
      "R1, 1kΩ 1W 100V ,\"device \"\"foo\"\"\",\"pack,age\",1.0,2.0,45.0,Top",
      lines[12].toStdString());
  EXPECT_EQ("R10,,device,\"pack,\"\"age\"\"\",-1.0,-2.0,315.0,Top",
            lines[13].toStdString());
  EXPECT_EQ("U5,1kΩ  ,device,package,1.0,2.0,45.0,Bottom",
            lines[14].toStdString());
  EXPECT_EQ("", lines[15].toStdString());
  EXPECT_EQ(16, lines.count());
}

TEST_F(PickPlaceCsvWriterTest, testTopSide) {
  std::shared_ptr<PickPlaceData> data = createData();
  PickPlaceCsvWriter writer(*data);
  writer.setIncludeMetadataComment(false);
  writer.setBoardSide(PickPlaceCsvWriter::BoardSide::TOP);
  std::shared_ptr<CsvFile> file = writer.generateCsv();
  QStringList lines = file->toString().split("\n");
  EXPECT_EQ(
      "Designator,Value,Device,Package,Position X,Position Y,Rotation,Side",
      lines[0].toStdString());
  EXPECT_EQ(
      "R1, 1kΩ 1W 100V ,\"device \"\"foo\"\"\",\"pack,age\",1.0,2.0,45.0,Top",
      lines[1].toStdString());
  EXPECT_EQ("R10,,device,\"pack,\"\"age\"\"\",-1.0,-2.0,315.0,Top",
            lines[2].toStdString());
  EXPECT_EQ("", lines[3].toStdString());
  EXPECT_EQ(4, lines.count());
}

TEST_F(PickPlaceCsvWriterTest, testBottomSide) {
  std::shared_ptr<PickPlaceData> data = createData();
  PickPlaceCsvWriter writer(*data);
  writer.setIncludeMetadataComment(false);
  writer.setBoardSide(PickPlaceCsvWriter::BoardSide::BOTTOM);
  std::shared_ptr<CsvFile> file = writer.generateCsv();
  QStringList lines = file->toString().split("\n");
  EXPECT_EQ(
      "Designator,Value,Device,Package,Position X,Position Y,Rotation,Side",
      lines[0].toStdString());
  EXPECT_EQ("U5,1kΩ  ,device,package,1.0,2.0,45.0,Bottom",
            lines[1].toStdString());
  EXPECT_EQ("", lines[2].toStdString());
  EXPECT_EQ(3, lines.count());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
