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
#include <librepcb/common/application.h>
#include <librepcb/library/sym/symbolpin.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SymbolPinTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(SymbolPinTest, testConstructFromSExpressionV01) {
  // Attention: Do NOT modify this string! It represents the freezed(!) file
  // format V0.1 and even current versions of LibrePCB must be able to load it!
  SExpression sexpr = SExpression::parse(
      "(pin d48b8bd2-a46c-4495-87a5-662747034098 (name \"1\")\n"
      " (position 1.234 2.345) (rotation 45.0) (length 0.5)\n"
      ")",
      FilePath());
  SymbolPin obj(sexpr, Version::fromString("0.1"));
  EXPECT_EQ(Uuid::fromString("d48b8bd2-a46c-4495-87a5-662747034098"),
            obj.getUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(UnsignedLength(500000), obj.getLength());
}

TEST_F(SymbolPinTest, testConstructFromSExpressionCurrentVersion) {
  SExpression sexpr = SExpression::parse(
      "(pin d48b8bd2-a46c-4495-87a5-662747034098 (name \"1\")\n"
      " (position 1.234 2.345) (rotation 45.0) (length 0.5)\n"
      ")",
      FilePath());
  SymbolPin obj(sexpr, qApp->getFileFormatVersion());
  EXPECT_EQ(Uuid::fromString("d48b8bd2-a46c-4495-87a5-662747034098"),
            obj.getUuid());
  EXPECT_EQ(Point(1234000, 2345000), obj.getPosition());
  EXPECT_EQ(Angle::deg45(), obj.getRotation());
  EXPECT_EQ(UnsignedLength(500000), obj.getLength());
}

TEST_F(SymbolPinTest, testSerializeAndDeserialize) {
  SymbolPin obj1(Uuid::createRandom(), CircuitIdentifier("foo"),
                 Point(123, 567), UnsignedLength(321), Angle(789));
  SExpression sexpr1 = obj1.serializeToDomElement("pin");

  SymbolPin obj2(sexpr1, qApp->getFileFormatVersion());
  SExpression sexpr2 = obj2.serializeToDomElement("pin");

  EXPECT_EQ(sexpr1.toByteArray(), sexpr2.toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace library
}  // namespace librepcb
