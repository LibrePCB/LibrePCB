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
#include <librepcb/core/export/gerberattribute.h>
#include <librepcb/core/types/angle.h>
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

class GerberAttributeTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(GerberAttributeTest, testUnset) {
  EXPECT_EQ("G04 #@! TD*\n",
            GerberAttribute::unset(QString()).toGerberString().toStdString());
  EXPECT_EQ("G04 #@! TD.Foo*\n",
            GerberAttribute::unset(".Foo").toGerberString().toStdString());
}

TEST_F(GerberAttributeTest, testFileGenerationSoftware) {
  EXPECT_EQ("G04 #@! TF.GenerationSoftware,Foo|Bar?!aou,Foo Bar,v1.0*\n",
            GerberAttribute::fileGenerationSoftware("Foo,|Bar%?!\\äöü",
                                                    "Foo Bar", "v1.0")
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileCreationDate) {
  EXPECT_EQ("G04 #@! TF.CreationDate,2000-02-01T01:02:03+01:00*\n",
            GerberAttribute::fileCreationDate(
                QDateTime(QDate(2000, 2, 1), QTime(1, 2, 3, 4),
                          Qt::OffsetFromUTC, 3600))
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileProjectId) {
  EXPECT_EQ(
      "G04 #@! TF.ProjectId,Project Name,"
      "bdf7bea5-b88e-41b2-be85-c1604e8ddfca,rev-1.0*\n",
      GerberAttribute::fileProjectId(
          "Project Name",
          Uuid::fromString("bdf7bea5-b88e-41b2-be85-c1604e8ddfca"), "rev-1.0")
          .toGerberString()
          .toStdString());
}

TEST_F(GerberAttributeTest, testFilePartSingle) {
  EXPECT_EQ("G04 #@! TF.Part,Single*\n",
            GerberAttribute::filePartSingle().toGerberString().toStdString());
}

TEST_F(GerberAttributeTest, testFileSameCoordinates) {
  EXPECT_EQ("G04 #@! TF.SameCoordinates*\n",
            GerberAttribute::fileSameCoordinates(QString())
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TF.SameCoordinates,asdf*\n",
            GerberAttribute::fileSameCoordinates("asdf")
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionProfile) {
  EXPECT_EQ("G04 #@! TF.FileFunction,Profile,P*\n",
            GerberAttribute::fileFunctionProfile(true)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TF.FileFunction,Profile,NP*\n",
            GerberAttribute::fileFunctionProfile(false)
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionCopper) {
  EXPECT_EQ(
      "G04 #@! TF.FileFunction,Copper,L1,Top*\n",
      GerberAttribute::fileFunctionCopper(1, GerberAttribute::CopperSide::Top)
          .toGerberString()
          .toStdString());
  EXPECT_EQ(
      "G04 #@! TF.FileFunction,Copper,L5,Inr*\n",
      GerberAttribute::fileFunctionCopper(5, GerberAttribute::CopperSide::Inner)
          .toGerberString()
          .toStdString());
  EXPECT_EQ("G04 #@! TF.FileFunction,Copper,L42,Bot*\n",
            GerberAttribute::fileFunctionCopper(
                42, GerberAttribute::CopperSide::Bottom)
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionSolderMask) {
  EXPECT_EQ(
      "G04 #@! TF.FileFunction,Soldermask,Top*\n",
      GerberAttribute::fileFunctionSolderMask(GerberAttribute::BoardSide::Top)
          .toGerberString()
          .toStdString());
  EXPECT_EQ("G04 #@! TF.FileFunction,Soldermask,Bot*\n",
            GerberAttribute::fileFunctionSolderMask(
                GerberAttribute::BoardSide::Bottom)
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionLegend) {
  EXPECT_EQ("G04 #@! TF.FileFunction,Legend,Top*\n",
            GerberAttribute::fileFunctionLegend(GerberAttribute::BoardSide::Top)
                .toGerberString()
                .toStdString());
  EXPECT_EQ(
      "G04 #@! TF.FileFunction,Legend,Bot*\n",
      GerberAttribute::fileFunctionLegend(GerberAttribute::BoardSide::Bottom)
          .toGerberString()
          .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionPaste) {
  EXPECT_EQ("G04 #@! TF.FileFunction,Paste,Top*\n",
            GerberAttribute::fileFunctionPaste(GerberAttribute::BoardSide::Top)
                .toGerberString()
                .toStdString());
  EXPECT_EQ(
      "G04 #@! TF.FileFunction,Paste,Bot*\n",
      GerberAttribute::fileFunctionPaste(GerberAttribute::BoardSide::Bottom)
          .toGerberString()
          .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionGlue) {
  EXPECT_EQ("G04 #@! TF.FileFunction,Glue,Top*\n",
            GerberAttribute::fileFunctionGlue(GerberAttribute::BoardSide::Top)
                .toGerberString()
                .toStdString());
  EXPECT_EQ(
      "G04 #@! TF.FileFunction,Glue,Bot*\n",
      GerberAttribute::fileFunctionGlue(GerberAttribute::BoardSide::Bottom)
          .toGerberString()
          .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionPlatedThroughHoleExcellon) {
  EXPECT_EQ("; #@! TF.FileFunction,Plated,2,5,PTH\n",
            GerberAttribute::fileFunctionPlatedThroughHole(2, 5)
                .toExcellonString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionNonPlatedThroughHoleExcellon) {
  EXPECT_EQ("; #@! TF.FileFunction,NonPlated,2,5,NPTH\n",
            GerberAttribute::fileFunctionNonPlatedThroughHole(2, 5)
                .toExcellonString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileFunctionMixedPlatingExcellon) {
  EXPECT_EQ("; #@! TF.FileFunction,MixedPlating,2,5\n",
            GerberAttribute::fileFunctionMixedPlating(2, 5)
                .toExcellonString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFilePolarity) {
  EXPECT_EQ("G04 #@! TF.FilePolarity,Positive*\n",
            GerberAttribute::filePolarity(GerberAttribute::Polarity::Positive)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TF.FilePolarity,Negative*\n",
            GerberAttribute::filePolarity(GerberAttribute::Polarity::Negative)
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testFileMd5) {
  EXPECT_EQ("G04 #@! TF.MD5,ASDF*\n",
            GerberAttribute::fileMd5("ASDF").toGerberString().toStdString());
}

TEST_F(GerberAttributeTest, testApertureFunction) {
  EXPECT_EQ("G04 #@! TA.AperFunction,Profile*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::Profile)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,Conductor*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::Conductor)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,NonConductor*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::NonConductor)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,ComponentPad*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::ComponentPad)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,SMDPad,CuDef*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::SmdPadCopperDefined)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,SMDPad,SMDef*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::SmdPadSolderMaskDefined)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,BGAPad,CuDef*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::BgaPadCopperDefined)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,BGAPad,SMDef*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::BgaPadSolderMaskDefined)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,ConnectorPad*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::ConnectorPad)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,HeatsinkPad*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::HeatsinkPad)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,ViaPad*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::ViaPad)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,TestPad*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::TestPad)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,FiducialPad,Local*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::FiducialPadLocal)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TA.AperFunction,FiducialPad,Global*\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::FiducialPadGlobal)
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testApertureFunctionExcellon) {
  EXPECT_EQ("; #@! TA.AperFunction,ViaDrill\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::ViaDrill)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,ComponentDrill\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::ComponentDrill)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,ComponentDrill,PressFit\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::ComponentDrillPressFit)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,MechanicalDrill\n",
            GerberAttribute::apertureFunction(
                GerberAttribute::ApertureFunction::MechanicalDrill)
                .toExcellonString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testApertureFunctionMixedPlatingDrillExcellon) {
  EXPECT_EQ("; #@! TA.AperFunction,NonPlated,NPTH,ViaDrill\n",
            GerberAttribute::apertureFunctionMixedPlatingDrill(
                false, GerberAttribute::ApertureFunction::ViaDrill)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,NonPlated,NPTH,ComponentDrill\n",
            GerberAttribute::apertureFunctionMixedPlatingDrill(
                false, GerberAttribute::ApertureFunction::ComponentDrill)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ(
      "; #@! TA.AperFunction,NonPlated,NPTH,ComponentDrill,PressFit\n",
      GerberAttribute::apertureFunctionMixedPlatingDrill(
          false, GerberAttribute::ApertureFunction::ComponentDrillPressFit)
          .toExcellonString()
          .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,NonPlated,NPTH,MechanicalDrill\n",
            GerberAttribute::apertureFunctionMixedPlatingDrill(
                false, GerberAttribute::ApertureFunction::MechanicalDrill)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,Plated,PTH,ViaDrill\n",
            GerberAttribute::apertureFunctionMixedPlatingDrill(
                true, GerberAttribute::ApertureFunction::ViaDrill)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,Plated,PTH,ComponentDrill\n",
            GerberAttribute::apertureFunctionMixedPlatingDrill(
                true, GerberAttribute::ApertureFunction::ComponentDrill)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,Plated,PTH,ComponentDrill,PressFit\n",
            GerberAttribute::apertureFunctionMixedPlatingDrill(
                true, GerberAttribute::ApertureFunction::ComponentDrillPressFit)
                .toExcellonString()
                .toStdString());
  EXPECT_EQ("; #@! TA.AperFunction,Plated,PTH,MechanicalDrill\n",
            GerberAttribute::apertureFunctionMixedPlatingDrill(
                true, GerberAttribute::ApertureFunction::MechanicalDrill)
                .toExcellonString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testObjectNet) {
  EXPECT_EQ("G04 #@! TO.N,*\n",
            GerberAttribute::objectNet("").toGerberString().toStdString());
  EXPECT_EQ("G04 #@! TO.N,N/C*\n",
            GerberAttribute::objectNet("N/C").toGerberString().toStdString());
  EXPECT_EQ(
      "G04 #@! TO.N,Foo Bar*\n",
      GerberAttribute::objectNet("Foo Bar").toGerberString().toStdString());
}

TEST_F(GerberAttributeTest, testObjectComponent) {
  EXPECT_EQ(
      "G04 #@! TO.C,C7*\n",
      GerberAttribute::objectComponent("C7").toGerberString().toStdString());
}

TEST_F(GerberAttributeTest, testObjectPin) {
  EXPECT_EQ("G04 #@! TO.P,C7,42*\n",
            GerberAttribute::objectPin("C7", "42", QString())
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TO.P,C7,42,VCC*\n",
            GerberAttribute::objectPin("C7", "42", "VCC")
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testComponentRotation) {
  EXPECT_EQ("G04 #@! TO.CRot,-90.0*\n",
            GerberAttribute::componentRotation(-Angle::deg90())
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TO.CRot,0.123456*\n",
            GerberAttribute::componentRotation(Angle(123456))
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testComponentManufacturer) {
  EXPECT_EQ("G04 #@! TO.CMfr,Foo \u00E4 \\u005C \\u0025 \\u002A \\u002C*\n",
            GerberAttribute::componentManufacturer("Foo\n\u00E4\r\n\\ % * ,")
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testComponentMpn) {
  EXPECT_EQ("G04 #@! TO.CMPN,Foo \u00E4 \\u005C \\u0025 \\u002A \\u002C*\n",
            GerberAttribute::componentMpn("Foo\n\u00E4\r\n\\ % * ,")
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testComponentValue) {
  EXPECT_EQ("G04 #@! TO.CVal,Foo \u00E4 \\u005C \\u0025 \\u002A \\u002C*\n",
            GerberAttribute::componentValue("Foo\n\u00E4\r\n\\ % * ,")
                .toGerberString()
                .toStdString());
}

TEST_F(GerberAttributeTest, testComponentMountType) {
  EXPECT_EQ("G04 #@! TO.CMnt,TH*\n",
            GerberAttribute::componentMountType(GerberAttribute::MountType::Tht)
                .toGerberString()
                .toStdString());
  EXPECT_EQ("G04 #@! TO.CMnt,SMD*\n",
            GerberAttribute::componentMountType(GerberAttribute::MountType::Smt)
                .toGerberString()
                .toStdString());
  EXPECT_EQ(
      "G04 #@! TO.CMnt,Fiducial*\n",
      GerberAttribute::componentMountType(GerberAttribute::MountType::Fiducial)
          .toGerberString()
          .toStdString());
  EXPECT_EQ(
      "G04 #@! TO.CMnt,Other*\n",
      GerberAttribute::componentMountType(GerberAttribute::MountType::Other)
          .toGerberString()
          .toStdString());
}

TEST_F(GerberAttributeTest, testComponentFootprint) {
  EXPECT_EQ("G04 #@! TO.CFtp,Foo \u00E4 \\u005C \\u0025 \\u002A \\u002C*\n",
            GerberAttribute::componentFootprint("Foo\n\u00E4\r\n\\ % * ,")
                .toGerberString()
                .toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
