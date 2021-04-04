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
#include <librepcb/common/cam/gerberattribute.h>
#include <librepcb/common/cam/gerberattributewriter.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GerberAttributeWriterTest : public ::testing::Test {
protected:
  static GerberAttribute functionConductor() noexcept {
    return GerberAttribute::apertureFunction(
        GerberAttribute::ApertureFunction::Conductor);
  }
  static GerberAttribute functionSmdPadCopperDefined() noexcept {
    return GerberAttribute::apertureFunction(
        GerberAttribute::ApertureFunction::SmdPadCopperDefined);
  }
  static GerberAttribute componentU1() noexcept {
    return GerberAttribute::objectComponent("U1");
  }
  static GerberAttribute componentU2() noexcept {
    return GerberAttribute::objectComponent("U2");
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(GerberAttributeWriterTest, testEmptyDictEmptyAttributes) {
  GerberAttributeWriter w;

  const char* expected = "";
  EXPECT_EQ(expected, w.setAttributes({}).toStdString());
}

TEST_F(GerberAttributeWriterTest, testEmptyDictNonEmptyAttributes) {
  GerberAttributeWriter w;

  const char* expected =
      "G04 #@! TA.AperFunction,Conductor*\n"
      "G04 #@! TO.C,U1*\n";
  EXPECT_EQ(
      expected,
      w.setAttributes({functionConductor(), componentU1()}).toStdString());
}

TEST_F(GerberAttributeWriterTest, testNonEmptyDictEmptyAttributes) {
  GerberAttributeWriter w;
  w.setAttributes({functionConductor(), componentU1()});

  const char* expected = "G04 #@! TD*\n";
  EXPECT_EQ(expected, w.setAttributes({}).toStdString());
}

TEST_F(GerberAttributeWriterTest, testNonEmptyDictSameAttributes) {
  GerberAttributeWriter w;
  w.setAttributes({functionConductor(), componentU1()});

  const char* expected = "";
  EXPECT_EQ(
      expected,
      w.setAttributes({functionConductor(), componentU1()}).toStdString());
}

TEST_F(GerberAttributeWriterTest, testNonEmptyDictPartlyDifferentAttributes) {
  GerberAttributeWriter w;
  w.setAttributes({functionConductor(), componentU1()});

  const char* expected = "G04 #@! TO.C,U2*\n";
  EXPECT_EQ(
      expected,
      w.setAttributes({functionConductor(), componentU2()}).toStdString());
}

TEST_F(GerberAttributeWriterTest, testNonEmptyDictFullyDifferentAttributes) {
  GerberAttributeWriter w;
  w.setAttributes({functionConductor(), componentU1()});

  const char* expected =
      "G04 #@! TA.AperFunction,SMDPad,CuDef*\n"
      "G04 #@! TO.C,U2*\n";
  EXPECT_EQ(expected,
            w.setAttributes({functionSmdPadCopperDefined(), componentU2()})
                .toStdString());
}

TEST_F(GerberAttributeWriterTest, testMoreAttributes) {
  GerberAttributeWriter w;
  w.setAttributes({functionConductor()});

  const char* expected = "G04 #@! TO.C,U1*\n";
  EXPECT_EQ(
      expected,
      w.setAttributes({componentU1(), functionConductor()}).toStdString());
}

TEST_F(GerberAttributeWriterTest, testLessAttributes) {
  GerberAttributeWriter w;
  w.setAttributes({functionConductor(), componentU1()});

  const char* expected = "G04 #@! TD.AperFunction*\n";
  EXPECT_EQ(expected, w.setAttributes({componentU1()}).toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
