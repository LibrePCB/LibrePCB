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
#include <librepcb/core/export/interactivehtmlbom.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class InteractiveHtmlBomTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(InteractiveHtmlBomTest, testViewModeSerialization) {
  for (auto x : {
           InteractiveHtmlBom::ViewMode::BomOnly,
           InteractiveHtmlBom::ViewMode::LeftRight,
           InteractiveHtmlBom::ViewMode::TopBottom,
       }) {
    const auto node = serialize(x);
    const auto value = deserialize<InteractiveHtmlBom::ViewMode>(*node);
    EXPECT_EQ(x, value);
  }
}

TEST_F(InteractiveHtmlBomTest, testHighlightPin1ModeSerialization) {
  for (auto x : {
           InteractiveHtmlBom::HighlightPin1Mode::None,
           InteractiveHtmlBom::HighlightPin1Mode::Selected,
           InteractiveHtmlBom::HighlightPin1Mode::All,
       }) {
    const auto node = serialize(x);
    const auto value =
        deserialize<InteractiveHtmlBom::HighlightPin1Mode>(*node);
    EXPECT_EQ(x, value);
  }
}

TEST_F(InteractiveHtmlBomTest, testGenerateHtml) {
  InteractiveHtmlBom ibom("Title", "Company", "Revision", "Date", Point(0, 0),
                          Point(100000000, 100000000));
  ibom.setFields({"Field 1", "Field 2"});

  ibom.addDrawing(
      InteractiveHtmlBom::DrawingKind::Polygon,
      InteractiveHtmlBom::DrawingLayer::Edge,
      Path::centeredRect(PositiveLength(100000), PositiveLength(100000)),
      UnsignedLength(0), false);
  ibom.addDrawing(
      InteractiveHtmlBom::DrawingKind::Polygon,
      InteractiveHtmlBom::DrawingLayer::SilkscreenFront,
      Path::centeredRect(PositiveLength(100000), PositiveLength(100000)),
      UnsignedLength(0), false);
  ibom.addDrawing(
      InteractiveHtmlBom::DrawingKind::ReferenceText,
      InteractiveHtmlBom::DrawingLayer::SilkscreenBack,
      Path::centeredRect(PositiveLength(100000), PositiveLength(100000)),
      UnsignedLength(100000), true);
  ibom.addDrawing(
      InteractiveHtmlBom::DrawingKind::Polygon,
      InteractiveHtmlBom::DrawingLayer::FabricationFront,
      Path::centeredRect(PositiveLength(100000), PositiveLength(100000)),
      UnsignedLength(100000), false);
  ibom.addDrawing(
      InteractiveHtmlBom::DrawingKind::ValueText,
      InteractiveHtmlBom::DrawingLayer::FabricationBack,
      Path::centeredRect(PositiveLength(100000), PositiveLength(100000)),
      UnsignedLength(0), true);

  ibom.addTrack(InteractiveHtmlBom::Layer::Top, Point(0, 0),
                Point(100000, 100000), PositiveLength(100000), std::nullopt);
  ibom.addTrack(InteractiveHtmlBom::Layer::Bottom, Point(0, 0),
                Point(100000, 100000), PositiveLength(100000), QString("net"));

  ibom.addVia({InteractiveHtmlBom::Layer::Top}, Point(0, 0),
              PositiveLength(2000000), PositiveLength(1000000), std::nullopt);
  ibom.addVia(
      {InteractiveHtmlBom::Layer::Top, InteractiveHtmlBom::Layer::Bottom},
      Point(100, 200), PositiveLength(2000000), PositiveLength(1000000),
      QString("net"));

  ibom.addPlaneFragment(
      InteractiveHtmlBom::Layer::Top,
      Path::centeredRect(PositiveLength(100000), PositiveLength(100000)),
      std::nullopt);
  ibom.addPlaneFragment(
      InteractiveHtmlBom::Layer::Bottom,
      Path::centeredRect(PositiveLength(100000), PositiveLength(100000)),
      QString("net"));

  const std::size_t id0 = ibom.addFootprint(
      InteractiveHtmlBom::Layer::Top, Point(0, 0), Angle::deg45(), Point(-5, 5),
      Point(5, -5), true, {"Value 2", "Value 2"},
      {
          InteractiveHtmlBom::Pad{
              true, true, Point(), Angle(), false, {}, {}, std::nullopt, false},
      });
  const std::size_t id1 = ibom.addFootprint(
      InteractiveHtmlBom::Layer::Bottom, Point(0, 0), Angle::deg45(),
      Point(-5, 5), Point(5, -5), false, {"Value 2", "Value 2"}, {});

  ibom.addBomRow(InteractiveHtmlBom::Sides::Top,
                 {std::make_pair(QString("R1"), id0)});
  ibom.addBomRow(InteractiveHtmlBom::Sides::Both,
                 {
                     std::make_pair(QString("R1"), id0),
                     std::make_pair(QString("R2"), id1),
                 });

  const QString html = ibom.generateHtml();
  EXPECT_TRUE(html.contains("<html"));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
