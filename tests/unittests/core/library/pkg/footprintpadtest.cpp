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
#include <librepcb/core/application.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class FootprintPadTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(FootprintPadTest, testFunctionsSerialization) {
  const std::vector<std::pair<FootprintPad::Function, QString>> items = {
      {FootprintPad::Function::Unspecified, "unspecified"},
      {FootprintPad::Function::StandardPad, "standard"},
      {FootprintPad::Function::PressFitPad, "pressfit"},
      {FootprintPad::Function::ThermalPad, "thermal"},
      {FootprintPad::Function::BgaPad, "bga"},
      {FootprintPad::Function::EdgeConnectorPad, "edge_connector"},
      {FootprintPad::Function::TestPad, "test"},
      {FootprintPad::Function::LocalFiducial, "local_fiducial"},
      {FootprintPad::Function::GlobalFiducial, "global_fiducial"},
  };
  for (const auto& item : items) {
    // Serialize
    auto sexpr = serialize(item.first);
    ASSERT_TRUE(sexpr != nullptr);
    EXPECT_EQ(item.second.toStdString(), sexpr->getValue().toStdString());

    // Deserialize
    sexpr = SExpression::createToken(item.second);
    EXPECT_EQ(item.first, deserialize<FootprintPad::Function>(*sexpr));
  }

  // In LibrePCB 1.x, "press_fit" shall also be deserializable.
  auto sexpr = SExpression::createToken("press_fit");
  if (Application::getFileFormatVersion() == Version::fromString("1")) {
    EXPECT_EQ(FootprintPad::Function::PressFitPad,
              deserialize<FootprintPad::Function>(*sexpr));
  } else {
    EXPECT_THROW(deserialize<FootprintPad::Function>(*sexpr), Exception);
  }
}

TEST_F(FootprintPadTest, testConstructFromSExpressionConnected) {
  std::unique_ptr<SExpression> sexpr = SExpression::parse(
      "(pad 7040952d-7016-49cd-8c3e-6078ecca98b9 (side top) (shape roundrect)\n"
      " (position 1.234 2.345) (rotation 45.0) (size 1.1 2.2) (radius 0.5)\n"
      " (stop_mask auto) (solder_paste 0.25) (clearance 0.33)"
      " (function unspecified)\n"
      " (package_pad d48b8bd2-a46c-4495-87a5-662747034098)\n"
      ")",
      FilePath());
  FootprintPad obj(*sexpr);
  EXPECT_EQ(Uuid::fromString("7040952d-7016-49cd-8c3e-6078ecca98b9"),
            obj.getUuid());
  EXPECT_EQ(Uuid::fromString("d48b8bd2-a46c-4495-87a5-662747034098"),
            obj.getPackagePadUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(FootprintPad::Shape::RoundedRect, obj.getShape());
  EXPECT_EQ(PositiveLength(1100000), obj.getWidth());
  EXPECT_EQ(UnsignedLength(2200000), obj.getHeight());
  EXPECT_EQ(UnsignedLimitedRatio(Ratio::fromPercent(50)), obj.getRadius());
  EXPECT_EQ(MaskConfig::automatic(), obj.getStopMaskConfig());
  EXPECT_EQ(MaskConfig::manual(Length(250000)), obj.getSolderPasteConfig());
  EXPECT_EQ(UnsignedLength(330000), obj.getCopperClearance());
  EXPECT_EQ(FootprintPad::ComponentSide::Top, obj.getComponentSide());
  EXPECT_EQ(FootprintPad::Function::Unspecified, obj.getFunction());
  EXPECT_EQ(0, obj.getHoles().count());
}

TEST_F(FootprintPadTest, testConstructFromSExpressionUnconnected) {
  std::unique_ptr<SExpression> sexpr = SExpression::parse(
      "(pad 7040952d-7016-49cd-8c3e-6078ecca98b9 (side bottom) (shape custom)\n"
      " (position 1.234 2.345) (rotation 45.0) (size 1.1 2.2) (radius 0.5)\n"
      " (stop_mask off) (solder_paste auto) (clearance 0.33)"
      " (function standard)\n"
      " (package_pad none)\n"
      " (vertex (position -1.1 -2.2) (angle 45.0))\n"
      " (vertex (position 1.1 -2.2) (angle 90.0))\n"
      " (vertex (position 0.0 2.2) (angle 0.0))\n"
      " (hole 7040952d-7016-49cd-8c3e-6078ecca98b9 (diameter 1.0)\n"
      "  (vertex (position 1.1 2.2) (angle 45.0))\n"
      " )\n"
      " (hole d48b8bd2-a46c-4495-87a5-662747034098 (diameter 2.0)\n"
      "  (vertex (position 3.3 4.4) (angle 0.0))\n"
      " )\n"
      ")",
      FilePath());
  FootprintPad obj(*sexpr);
  EXPECT_EQ(Uuid::fromString("7040952d-7016-49cd-8c3e-6078ecca98b9"),
            obj.getUuid());
  EXPECT_EQ(tl::nullopt, obj.getPackagePadUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(FootprintPad::Shape::Custom, obj.getShape());
  EXPECT_EQ(PositiveLength(1100000), obj.getWidth());
  EXPECT_EQ(UnsignedLength(2200000), obj.getHeight());
  EXPECT_EQ(UnsignedLimitedRatio(Ratio::fromPercent(50)), obj.getRadius());
  EXPECT_EQ(MaskConfig::off(), obj.getStopMaskConfig());
  EXPECT_EQ(MaskConfig::automatic(), obj.getSolderPasteConfig());
  EXPECT_EQ(UnsignedLength(330000), obj.getCopperClearance());
  EXPECT_EQ(FootprintPad::ComponentSide::Bottom, obj.getComponentSide());
  EXPECT_EQ(FootprintPad::Function::StandardPad, obj.getFunction());
  EXPECT_EQ(3, obj.getCustomShapeOutline().getVertices().count());
  EXPECT_EQ(2, obj.getHoles().count());
}

TEST_F(FootprintPadTest, testSerializeAndDeserialize) {
  FootprintPad obj1(
      Uuid::createRandom(), Uuid::createRandom(), Point(123, 567), Angle(789),
      FootprintPad::Shape::RoundedOctagon, PositiveLength(123),
      PositiveLength(456), UnsignedLimitedRatio(Ratio::fromPercent(50)),
      Path({Vertex(Point(1, 2), Angle(3)), Vertex(Point(4, 5), Angle(6))}),
      MaskConfig::automatic(), MaskConfig::manual(Length(123456)),
      UnsignedLength(98765), FootprintPad::ComponentSide::Top,
      FootprintPad::Function::Unspecified,
      PadHoleList{
          std::make_shared<PadHole>(Uuid::createRandom(),
                                    PositiveLength(100000),
                                    makeNonEmptyPath(Point(100, 200))),
          std::make_shared<PadHole>(Uuid::createRandom(),
                                    PositiveLength(200000),
                                    makeNonEmptyPath(Point(300, 400))),
      });
  std::unique_ptr<SExpression> sexpr1 = SExpression::createList("obj");
  obj1.serialize(*sexpr1);

  FootprintPad obj2(*sexpr1);
  std::unique_ptr<SExpression> sexpr2 = SExpression::createList("obj");
  obj2.serialize(*sexpr2);

  EXPECT_EQ(sexpr1->toByteArray(), sexpr2->toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
