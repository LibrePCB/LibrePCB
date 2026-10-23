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
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/attribute/attrtypefrequency.h>
#include <librepcb/core/attribute/attrtypevoltage.h>
#include <librepcb/core/serialization/sexpression.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AttributeTypeTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(AttributeTypeTest, testSerialize) {
  const AttributeType& type = AttrTypeVoltage::instance();
  EXPECT_EQ("voltage\n", serialize(type)->toByteArray());
}

TEST_F(AttributeTypeTest, testTryExtractUnitPrefersLongestSuffix) {
  const AttributeType& type = AttrTypeFrequency::instance();
  for (const auto& [input, unit] :
       {std::make_pair("12MHz", "megahertz"),
        std::make_pair("32kHz", "kilohertz"),
        std::make_pair("1GHz", "gigahertz"), std::make_pair("50Hz", "hertz")}) {
    QString value = input;
    const AttributeUnit* extracted = type.tryExtractUnitFromValue(value);
    ASSERT_NE(nullptr, extracted) << input;
    EXPECT_EQ(unit, extracted->getName().toStdString()) << input;
    EXPECT_TRUE(type.isValueValid(value)) << input;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
