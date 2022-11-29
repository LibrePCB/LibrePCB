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
#include <librepcb/core/geometry/trace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class TraceTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(TraceTest, testConstructFromSExpressionV01) {
  // Attention: Do NOT modify this string! It represents the freezed(!) file
  // format V0.1 and even current versions of LibrePCB must be able to load it!
  SExpression sexpr = SExpression::parse(
      "(trace c893f5a0-3fec-498b-99d6-467d5d69825d (layer bot_cu) (width 0.5) "
      "(from (device 0d8f2ef9-34f4-4400-a313-f17cdcdfe924) "
      "(pad 65ab6c75-b264-4fed-b445-d3d98c956008)) "
      "(to (via 1e80206f-158b-48e6-9cb4-6e368af7b7d7)))",
      FilePath());
  Trace obj(sexpr, Version::fromString("0.1"));
  EXPECT_EQ(Uuid::fromString("c893f5a0-3fec-498b-99d6-467d5d69825d"),
            obj.getUuid());
  EXPECT_EQ(GraphicsLayerName("bot_cu"), obj.getLayer());
  EXPECT_EQ(PositiveLength(500000), obj.getWidth());
  EXPECT_EQ(TraceAnchor::pad(
                Uuid::fromString("0d8f2ef9-34f4-4400-a313-f17cdcdfe924"),
                Uuid::fromString("65ab6c75-b264-4fed-b445-d3d98c956008")),
            obj.getStartPoint());
  EXPECT_EQ(TraceAnchor::via(
                Uuid::fromString("1e80206f-158b-48e6-9cb4-6e368af7b7d7")),
            obj.getEndPoint());
}

TEST_F(TraceTest, testConstructFromSExpressionCurrentVersion) {
  SExpression sexpr = SExpression::parse(
      "(trace c893f5a0-3fec-498b-99d6-467d5d69825d (layer bot_cu) (width 0.5) "
      "(from (device 0d8f2ef9-34f4-4400-a313-f17cdcdfe924) "
      "(pad 65ab6c75-b264-4fed-b445-d3d98c956008)) "
      "(to (via 1e80206f-158b-48e6-9cb4-6e368af7b7d7)))",
      FilePath());
  Trace obj(sexpr, qApp->getFileFormatVersion());
  EXPECT_EQ(Uuid::fromString("c893f5a0-3fec-498b-99d6-467d5d69825d"),
            obj.getUuid());
  EXPECT_EQ(GraphicsLayerName("bot_cu"), obj.getLayer());
  EXPECT_EQ(PositiveLength(500000), obj.getWidth());
  EXPECT_EQ(TraceAnchor::pad(
                Uuid::fromString("0d8f2ef9-34f4-4400-a313-f17cdcdfe924"),
                Uuid::fromString("65ab6c75-b264-4fed-b445-d3d98c956008")),
            obj.getStartPoint());
  EXPECT_EQ(TraceAnchor::via(
                Uuid::fromString("1e80206f-158b-48e6-9cb4-6e368af7b7d7")),
            obj.getEndPoint());
}

TEST_F(TraceTest, testSerializeAndDeserialize) {
  Trace obj1(Uuid::createRandom(), GraphicsLayerName("foo"),
             PositiveLength(123), TraceAnchor::junction(Uuid::createRandom()),
             TraceAnchor::pad(Uuid::createRandom(), Uuid::createRandom()));
  SExpression sexpr1 = SExpression::createList("obj");
  obj1.serialize(sexpr1);

  Trace obj2(sexpr1, qApp->getFileFormatVersion());
  SExpression sexpr2 = SExpression::createList("obj");
  obj2.serialize(sexpr2);

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
