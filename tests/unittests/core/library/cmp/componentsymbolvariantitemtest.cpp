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
#include <librepcb/core/library/cmp/componentsymbolvariantitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ComponentSymbolVariantItemTest : public ::testing::Test {
protected:
  ComponentSymbolVariantItem mObj1;
  ComponentSymbolVariantItem mObj2;

  ComponentSymbolVariantItemTest()
    : mObj1(Uuid::fromString("c53e0493-d446-4c97-a302-95d62840c762"),
            Uuid::fromString("d2f47222-2c1c-4097-8611-9559c3198fdf"),
            Point(12, 34), Angle(42), true,
            ComponentSymbolVariantItemSuffix("A")),
      mObj2(Uuid::fromString("0e97ed2c-b7c8-40e5-aa9e-194cde326c3e"),
            Uuid::fromString("a2b53202-e2a8-488e-9847-a742d093e4be"),
            Point(56, 78), Angle(24), false,
            ComponentSymbolVariantItemSuffix("B")) {}
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ComponentSymbolVariantItemTest, testOperatorAssignment) {
  mObj2 = mObj1;
  EXPECT_EQ(mObj1.getUuid(), mObj2.getUuid());
  EXPECT_EQ(mObj1.getSymbolUuid(), mObj2.getSymbolUuid());
  EXPECT_EQ(mObj1.getSymbolPosition(), mObj2.getSymbolPosition());
  EXPECT_EQ(mObj1.getSymbolRotation(), mObj2.getSymbolRotation());
  EXPECT_EQ(mObj1.isRequired(), mObj2.isRequired());
  EXPECT_EQ(mObj1.getSuffix(), mObj2.getSuffix());
}

TEST_F(ComponentSymbolVariantItemTest, testOperatorEqual) {
  ComponentSymbolVariantItem obj(
      mObj1.getUuid(), mObj1.getSymbolUuid(), mObj1.getSymbolPosition(),
      mObj1.getSymbolRotation(), mObj1.isRequired(), mObj1.getSuffix());
  EXPECT_TRUE(obj == mObj1);
  EXPECT_FALSE(obj != mObj1);
}

TEST_F(ComponentSymbolVariantItemTest, testOperatorEqualOnDifferentUuid) {
  ComponentSymbolVariantItem obj(
      mObj2.getUuid(), mObj1.getSymbolUuid(), mObj1.getSymbolPosition(),
      mObj1.getSymbolRotation(), mObj1.isRequired(), mObj1.getSuffix());
  EXPECT_FALSE(obj == mObj1);
  EXPECT_TRUE(obj != mObj1);
}

TEST_F(ComponentSymbolVariantItemTest, testOperatorEqualOnDifferentSymbol) {
  ComponentSymbolVariantItem obj(
      mObj1.getUuid(), mObj2.getSymbolUuid(), mObj1.getSymbolPosition(),
      mObj1.getSymbolRotation(), mObj1.isRequired(), mObj1.getSuffix());
  EXPECT_FALSE(obj == mObj1);
  EXPECT_TRUE(obj != mObj1);
}

TEST_F(ComponentSymbolVariantItemTest, testOperatorEqualOnDifferentPos) {
  ComponentSymbolVariantItem obj(
      mObj1.getUuid(), mObj1.getSymbolUuid(), mObj2.getSymbolPosition(),
      mObj1.getSymbolRotation(), mObj1.isRequired(), mObj1.getSuffix());
  EXPECT_FALSE(obj == mObj1);
  EXPECT_TRUE(obj != mObj1);
}

TEST_F(ComponentSymbolVariantItemTest, testOperatorEqualOnDifferentRot) {
  ComponentSymbolVariantItem obj(
      mObj1.getUuid(), mObj1.getSymbolUuid(), mObj1.getSymbolPosition(),
      mObj2.getSymbolRotation(), mObj1.isRequired(), mObj1.getSuffix());
  EXPECT_FALSE(obj == mObj1);
  EXPECT_TRUE(obj != mObj1);
}

TEST_F(ComponentSymbolVariantItemTest, testOperatorEqualOnDifferentRequired) {
  ComponentSymbolVariantItem obj(
      mObj1.getUuid(), mObj1.getSymbolUuid(), mObj1.getSymbolPosition(),
      mObj1.getSymbolRotation(), mObj2.isRequired(), mObj1.getSuffix());
  EXPECT_FALSE(obj == mObj1);
  EXPECT_TRUE(obj != mObj1);
}

TEST_F(ComponentSymbolVariantItemTest, testOperatorEqualOnDifferentSuffix) {
  ComponentSymbolVariantItem obj(
      mObj1.getUuid(), mObj1.getSymbolUuid(), mObj1.getSymbolPosition(),
      mObj1.getSymbolRotation(), mObj1.isRequired(), mObj2.getSuffix());
  EXPECT_FALSE(obj == mObj1);
  EXPECT_TRUE(obj != mObj1);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
