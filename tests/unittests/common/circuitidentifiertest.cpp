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
#include <librepcb/common/circuitidentifier.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  QString input;
  bool valid;
} CircuitIdentifierTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class CircuitIdentifierTest
  : public ::testing::TestWithParam<CircuitIdentifierTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(CircuitIdentifierTest, testConstructor) {
  const CircuitIdentifierTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, *CircuitIdentifier(data.input));
  } else {
    EXPECT_THROW(CircuitIdentifier(data.input), RuntimeError);
  }
}

TEST_P(CircuitIdentifierTest, testClean) {
  const CircuitIdentifierTestData& data = GetParam();

  if (data.valid) {
    EXPECT_EQ(data.input, cleanCircuitIdentifier(data.input));
  } else {
    QString cleaned = cleanCircuitIdentifier(data.input);
    if (!cleaned.isEmpty()) {
      CircuitIdentifier identifier(cleaned);  // must not throw
    }
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(CircuitIdentifierTest, CircuitIdentifierTest, ::testing::Values(
    // valid identifiers
    CircuitIdentifierTestData({"1", true}),
    CircuitIdentifierTestData({"A", true}),
    CircuitIdentifierTestData({"z", true}),
    CircuitIdentifierTestData({"_", true}),
    CircuitIdentifierTestData({"+", true}),
    CircuitIdentifierTestData({"-", true}),
    CircuitIdentifierTestData({"01234567890123456789012345678901", true}),
    CircuitIdentifierTestData({"_+-/!?@#$asDF1234", true}),

    // invalid identifiers
    CircuitIdentifierTestData({"", false}), // empty
    CircuitIdentifierTestData({"012345678901234567890123456789012", false}), // too long
    CircuitIdentifierTestData({" ", false}), // space
    CircuitIdentifierTestData({"A B", false}), // space
    CircuitIdentifierTestData({";", false}), // invalid character
    CircuitIdentifierTestData({":1234", false}), // invalid character at start
    CircuitIdentifierTestData({"AS:df", false}), // invalid character in the middle
    CircuitIdentifierTestData({"1234:", false}), // invalid character at end
    CircuitIdentifierTestData({"\n", false}), // invalid character
    CircuitIdentifierTestData({"Foo\tBar", false}), // invalid character in the middle
    CircuitIdentifierTestData({"Foo\nBar", false}), // invalid character in the middle
    CircuitIdentifierTestData({"\nFoo", false}), // invalid character at start
    CircuitIdentifierTestData({"Foo\n", false}) // invalid character at end
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
