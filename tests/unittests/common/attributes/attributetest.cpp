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
#include <librepcb/common/attributes/attribute.h>
#include <librepcb/common/attributes/attrtypevoltage.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  QString key;
  QString type;
  QString unit;
  QString value;
  QByteArray serialized;
  bool validSExpression;
} AttributeTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AttributeTest : public ::testing::TestWithParam<AttributeTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(AttributeTest, testConstructFromSExpression) {
  const AttributeTestData& data = GetParam();

  const AttributeType& type = AttributeType::fromString(data.type);
  Attribute attribute(AttributeKey(data.key), type, data.value,
                      type.getUnitFromString(data.unit));
  SExpression sexpr = SExpression::parse(data.serialized, FilePath());

  if (data.validSExpression) {
    EXPECT_EQ(attribute, Attribute(sexpr));
  } else {
    EXPECT_THROW({ Attribute a(sexpr); }, Exception);
  }
}

TEST_P(AttributeTest, testSerialize) {
  const AttributeTestData& data = GetParam();

  const AttributeType& type = AttributeType::fromString(data.type);
  Attribute attribute(AttributeKey(data.key), type, data.value,
                      type.getUnitFromString(data.unit));

  if (data.validSExpression) {
    SExpression sexpr = attribute.serializeToDomElement("attribute");
    EXPECT_EQ(data.serialized, sexpr.toByteArray());
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(AttributeTest, AttributeTest, ::testing::Values(
  // Invalid serialization
  AttributeTestData({"FOO", "voltage", "volt", "4.2",
                     "(attribute \"FOO\" (type foo) (unit volt) (value \"4.2\"))\n",
                     false}),
  AttributeTestData({"FOO", "voltage", "volt", "4.2",
                     "(attribute \"FOO\" (type voltage) (unit volt) (value \"foo\"))\n",
                     false}),
  AttributeTestData({"FOO", "voltage", "volt", "4.2",
                     "(attribute \"FOO\" (type voltage) (unit foo) (value \"4.2\"))\n",
                     false}),
  AttributeTestData({"FOO", "voltage", "volt", "4.2",
                     "(attribute (type voltage) (unit foo) (value \"4.2\"))\n",
                     false}),
  AttributeTestData({"FOO", "voltage", "volt", "4.2",
                     "(attribute \"\" (type voltage) (unit volt) (value \"4.2\"))\n",
                     false}),
  // Valid serialization
  AttributeTestData({"FOO", "voltage", "volt", "4.2",
                     "(attribute \"FOO\" (type voltage) (unit volt) (value \"4.2\"))\n",
                     true})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
