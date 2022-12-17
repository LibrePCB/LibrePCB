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
#include <librepcb/core/export/excellongenerator.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ExcellonGeneratorTest : public ::testing::Test {
protected:
  std::string makeComparable(QString str) const noexcept {
    // replace volatile data in exported files with well-known, constant data
    str.replace(QRegularExpression(
                    "TF\\.GenerationSoftware,LibrePCB,LibrePCB,[^\\s\\*]*"),
                "TF.GenerationSoftware,LibrePCB,LibrePCB,0.1.2");
    str.replace(QRegularExpression("TF\\.CreationDate,[^\\s\\*]*"),
                "TF.CreationDate,2019-01-02T03:04:05");
    return str.toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ExcellonGeneratorTest, testCircularDrills) {
  ExcellonGenerator gen(
      QDateTime(QDate(2000, 2, 1), QTime(1, 2, 3, 4), Qt::OffsetFromUTC, 3600),
      "My Project", Uuid::fromString("bdf7bea5-b88e-41b2-be85-c1604e8ddfca"),
      "1.0", ExcellonGenerator::Plating::Mixed, 1, 4);

  gen.drill(Point(111, 222), PositiveLength(500000), true,
            ExcellonGenerator::Function::ComponentDrill);
  gen.drill(Point(333, 444), PositiveLength(600000), false,
            ExcellonGenerator::Function::MechanicalDrill);
  gen.drill(makeNonEmptyPath(Point(555, 666)), PositiveLength(500000), true,
            ExcellonGenerator::Function::ComponentDrill);

  gen.generate();
  EXPECT_EQ(
      "M48\n"
      "; #@! TF.GenerationSoftware,LibrePCB,LibrePCB,0.1.2\n"
      "; #@! TF.CreationDate,2019-01-02T03:04:05\n"
      "; #@! TF.ProjectId,My Project,bdf7bea5-b88e-41b2-be85-c1604e8ddfca,1.0\n"
      "; #@! TF.Part,Single\n"
      "; #@! TF.SameCoordinates\n"
      "; #@! TF.FileFunction,MixedPlating,1,4\n"
      "FMAT,2\n"
      "METRIC,TZ\n"
      "; #@! TA.AperFunction,Plated,PTH,ComponentDrill\n"
      "T1C0.5\n"
      "; #@! TA.AperFunction,NonPlated,NPTH,MechanicalDrill\n"
      "T2C0.6\n"
      "%\n"
      "G90\n"
      "G05\n"
      "M71\n"
      "T1\n"
      "X0.000555Y0.000666\n"
      "X0.000111Y0.000222\n"
      "T2\n"
      "X0.000333Y0.000444\n"
      "T0\n"
      "M30\n",
      makeComparable(gen.toStr()));
}

TEST_F(ExcellonGeneratorTest, testSlotRout) {
  ExcellonGenerator gen(
      QDateTime(QDate(2000, 2, 1), QTime(1, 2, 3, 4), Qt::OffsetFromUTC, 3600),
      "My Project", Uuid::fromString("bdf7bea5-b88e-41b2-be85-c1604e8ddfca"),
      "1.0", ExcellonGenerator::Plating::Mixed, 1, 4);

  gen.drill(NonEmptyPath(Path({
                Vertex(Point(111, 222), Angle::deg90()),
                Vertex(Point(333, 444), Angle::deg0()),
                Vertex(Point(555, 666), Angle::deg0()),
            })),
            PositiveLength(500000), false,
            ExcellonGenerator::Function::MechanicalDrill);

  gen.generate();
  EXPECT_EQ(
      "M48\n"
      "; #@! TF.GenerationSoftware,LibrePCB,LibrePCB,0.1.2\n"
      "; #@! TF.CreationDate,2019-01-02T03:04:05\n"
      "; #@! TF.ProjectId,My Project,bdf7bea5-b88e-41b2-be85-c1604e8ddfca,1.0\n"
      "; #@! TF.Part,Single\n"
      "; #@! TF.SameCoordinates\n"
      "; #@! TF.FileFunction,MixedPlating,1,4\n"
      "FMAT,2\n"
      "METRIC,TZ\n"
      "; #@! TA.AperFunction,NonPlated,NPTH,MechanicalDrill\n"
      "T1C0.5\n"
      "%\n"
      "G90\n"
      "G05\n"
      "M71\n"
      "T1\n"
      "G00X0.000111Y0.000222\n"
      "M15\n"
      "G03X0.000333Y0.000444A0.000222\n"
      "G01X0.000555Y0.000666\n"
      "M16\n"
      "G05\n"
      "T0\n"
      "M30\n",
      makeComparable(gen.toStr()));
}

TEST_F(ExcellonGeneratorTest, testSlotG85) {
  ExcellonGenerator gen(
      QDateTime(QDate(2000, 2, 1), QTime(1, 2, 3, 4), Qt::OffsetFromUTC, 3600),
      "My Project", Uuid::fromString("bdf7bea5-b88e-41b2-be85-c1604e8ddfca"),
      "1.0", ExcellonGenerator::Plating::Mixed, 1, 4);
  gen.setUseG85Slots(true);

  gen.drill(NonEmptyPath(Path({
                Vertex(Point(111, 222), Angle::deg0()),
                Vertex(Point(333, 444), Angle::deg0()),
                Vertex(Point(555, 666), Angle::deg0()),
            })),
            PositiveLength(500000), false,
            ExcellonGenerator::Function::MechanicalDrill);

  gen.generate();
  EXPECT_EQ(
      "M48\n"
      "; #@! TF.GenerationSoftware,LibrePCB,LibrePCB,0.1.2\n"
      "; #@! TF.CreationDate,2019-01-02T03:04:05\n"
      "; #@! TF.ProjectId,My Project,bdf7bea5-b88e-41b2-be85-c1604e8ddfca,1.0\n"
      "; #@! TF.Part,Single\n"
      "; #@! TF.SameCoordinates\n"
      "; #@! TF.FileFunction,MixedPlating,1,4\n"
      "FMAT,2\n"
      "METRIC,TZ\n"
      "; #@! TA.AperFunction,NonPlated,NPTH,MechanicalDrill\n"
      "T1C0.5\n"
      "%\n"
      "G90\n"
      "G05\n"
      "M71\n"
      "T1\n"
      "X0.000111Y0.000222G85X0.000333Y0.000444\n"
      "X0.000333Y0.000444G85X0.000555Y0.000666\n"
      "T0\n"
      "M30\n",
      makeComparable(gen.toStr()));
}

TEST_F(ExcellonGeneratorTest, testCurvedSlotG85) {
  ExcellonGenerator gen(
      QDateTime(QDate(2000, 2, 1), QTime(1, 2, 3, 4), Qt::OffsetFromUTC, 3600),
      "My Project", Uuid::fromString("bdf7bea5-b88e-41b2-be85-c1604e8ddfca"),
      "1.0", ExcellonGenerator::Plating::Mixed, 1, 4);
  gen.setUseG85Slots(true);

  gen.drill(NonEmptyPath(Path({
                Vertex(Point(111, 222), Angle::deg90()),
                Vertex(Point(333, 444), Angle::deg0()),
                Vertex(Point(555, 666), Angle::deg0()),
            })),
            PositiveLength(500000), false,
            ExcellonGenerator::Function::MechanicalDrill);

  EXPECT_THROW(gen.generate(), RuntimeError);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
