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
#include <librepcb/core/geometry/trace.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/layer.h>

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

TEST_F(TraceTest, testAnchorLessThan) {
  // The comparison operator is relevant for the file format.
  const QVector<TraceAnchor> input = {
      TraceAnchor::junction(
          Uuid::fromString("5bed2074-1b02-4db5-9b0e-293c42d8728f")),
      TraceAnchor::footprintPad(
          Uuid::fromString("d14141ff-651f-40f0-87be-b4f86831375a"),
          Uuid::fromString("65ab6c75-b264-4fed-b445-d3d98c956008")),
      TraceAnchor::pad(
          Uuid::fromString("e706fdf8-4ced-4cc4-a49f-757ca395272b")),
      TraceAnchor::via(
          Uuid::fromString("c893f5a0-3fec-498b-99d6-467d5d69825d")),
      TraceAnchor::footprintPad(
          Uuid::fromString("94ca7c55-bf86-43e0-8399-d713ce1f1929"),
          Uuid::fromString("65ab6c75-b264-4fed-b445-d3d98c956008")),
      TraceAnchor::junction(
          Uuid::fromString("0d8f2ef9-34f4-4400-a313-f17cdcdfe924")),
      TraceAnchor::via(
          Uuid::fromString("1e80206f-158b-48e6-9cb4-6e368af7b7d7")),
      TraceAnchor::pad(
          Uuid::fromString("70c4ec26-2d47-441d-beeb-43aa968b4d2e")),
      TraceAnchor::footprintPad(
          Uuid::fromString("94ca7c55-bf86-43e0-8399-d713ce1f1929"),
          Uuid::fromString("04bb6ac3-34d7-4fb3-b274-44f845f8d3b5")),
  };
  const QVector<TraceAnchor> expected = {
      TraceAnchor::footprintPad(
          Uuid::fromString("94ca7c55-bf86-43e0-8399-d713ce1f1929"),
          Uuid::fromString("04bb6ac3-34d7-4fb3-b274-44f845f8d3b5")),
      TraceAnchor::footprintPad(
          Uuid::fromString("94ca7c55-bf86-43e0-8399-d713ce1f1929"),
          Uuid::fromString("65ab6c75-b264-4fed-b445-d3d98c956008")),
      TraceAnchor::footprintPad(
          Uuid::fromString("d14141ff-651f-40f0-87be-b4f86831375a"),
          Uuid::fromString("65ab6c75-b264-4fed-b445-d3d98c956008")),
      TraceAnchor::pad(
          Uuid::fromString("70c4ec26-2d47-441d-beeb-43aa968b4d2e")),
      TraceAnchor::pad(
          Uuid::fromString("e706fdf8-4ced-4cc4-a49f-757ca395272b")),
      TraceAnchor::via(
          Uuid::fromString("1e80206f-158b-48e6-9cb4-6e368af7b7d7")),
      TraceAnchor::via(
          Uuid::fromString("c893f5a0-3fec-498b-99d6-467d5d69825d")),
      TraceAnchor::junction(
          Uuid::fromString("0d8f2ef9-34f4-4400-a313-f17cdcdfe924")),
      TraceAnchor::junction(
          Uuid::fromString("5bed2074-1b02-4db5-9b0e-293c42d8728f")),
  };

  QVector<TraceAnchor> actual = input;
  std::sort(actual.begin(), actual.end());
  EXPECT_EQ(expected, actual);
}

TEST_F(TraceTest, testConstructFromSExpression) {
  std::unique_ptr<SExpression> sexpr = SExpression::parse(
      "(trace c893f5a0-3fec-498b-99d6-467d5d69825d (layer bot_cu) (width 0.5) "
      "(from (via 1e80206f-158b-48e6-9cb4-6e368af7b7d7)) "
      "(to (device 0d8f2ef9-34f4-4400-a313-f17cdcdfe924) "
      "(pad 65ab6c75-b264-4fed-b445-d3d98c956008)))",
      FilePath());
  Trace obj(*sexpr);
  EXPECT_EQ(Uuid::fromString("c893f5a0-3fec-498b-99d6-467d5d69825d"),
            obj.getUuid());
  EXPECT_EQ("bot_cu", obj.getLayer().getId());
  EXPECT_EQ(PositiveLength(500000), obj.getWidth());
  EXPECT_EQ(TraceAnchor::footprintPad(
                Uuid::fromString("0d8f2ef9-34f4-4400-a313-f17cdcdfe924"),
                Uuid::fromString("65ab6c75-b264-4fed-b445-d3d98c956008")),
            obj.getP1());
  EXPECT_EQ(TraceAnchor::via(
                Uuid::fromString("1e80206f-158b-48e6-9cb4-6e368af7b7d7")),
            obj.getP2());
}

TEST_F(TraceTest, testSerializeAndDeserialize) {
  Trace obj1(Uuid::createRandom(), Layer::topCopper(), PositiveLength(123),
             TraceAnchor::junction(Uuid::createRandom()),
             TraceAnchor::pad(Uuid::createRandom()));
  std::unique_ptr<SExpression> sexpr1 = SExpression::createList("obj");
  obj1.serialize(*sexpr1);

  Trace obj2(*sexpr1);
  std::unique_ptr<SExpression> sexpr2 = SExpression::createList("obj");
  obj2.serialize(*sexpr2);

  EXPECT_EQ(sexpr1->toByteArray(), sexpr2->toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
