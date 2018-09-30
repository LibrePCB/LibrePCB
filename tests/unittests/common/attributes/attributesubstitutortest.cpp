/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#include "attributeproviderdummy.h"

#include <gtest/gtest.h>
#include <librepcb/common/attributes/attributesubstitutor.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  const QString input;
  const QString output;
} AttributeSubstitutorTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AttributeSubstitutorTest
  : public ::testing::TestWithParam<AttributeSubstitutorTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(AttributeSubstitutorTest, testData) {
  const AttributeSubstitutorTestData& data = GetParam();

  AttributeProviderDummy ap;
  QString output = AttributeSubstitutor::substitute(data.input, &ap);
  EXPECT_EQ(data.output, output)
      << "Actual value: '" << qPrintable(output) << "'";
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

using ASTD = AttributeSubstitutorTestData;

// clang-format off
INSTANTIATE_TEST_CASE_P(AttributeSubstitutorTest, AttributeSubstitutorTest, ::testing::Values(
    ASTD({"",                                   ""}),
    ASTD({"Hello { World! }} {{",               "Hello { World! }} {{"}),
    ASTD({"{{NONEXISTENT}}",                    ""}),
    ASTD({"{{KEY}}",                            ""}),
    ASTD({"{{KEY_1}}",                          "Normal value"}),
    //ASTD({"{{KEY_1}} {{KEY_1}}",                "Normal value Normal value"}),
    ASTD({"some {}}}{{ noise",                  "some {}}}{{ noise"}),
    ASTD({"{{KEY_2}}",                          "Value with {}}}{{ noise"}),
    ASTD({"{{KEY_3}}",                          "Recursive  value"}),
    ASTD({"{{KEY_4}}",                          "Recursive Normal value value"}),
    ASTD({"{{KEY_5}}",                          "Recursive Recursive Normal value value value"}),
    ASTD({"{{KEY_6}}",                          "Endless Endless  part 2 part 1"}),
    ASTD({"{{KEY_7}}",                          "Endless Endless  part 1 part 2"}),
    ASTD({"Foo {KEY_7 }}{{KEY_7}} {{KEYY}}",    "Foo {KEY_7 }}Endless Endless  part 1 part 2 "}),
    ASTD({"{{KEY_3}} foo{ { KEY_5}} {{KEY}}",   "Recursive  value foo{ { KEY_5}} "}),
    ASTD({"{{KEY_1}} {{KEY_2 or KEY_3}} foo",   "Normal value Value with {}}}{{ noise foo"}),
    //ASTD({"{{KEY_8 or KEY_1}}",                 "Normal value"}),
    //ASTD({"{{KEY or KEY_4 or KEY_3}} {{KEY_1}}","Recursive Normal value value Normal value"}),
    //ASTD({"{{KEY_1}} {{FOO or KEY or KEY_5}}!", "Normal value Recursive Recursive Normal value value value!"}),
    ASTD({"{{FOO or BAR or BAR or FOO}}",       ""}),
    ASTD({"{{FOO or BAR or KEY or KEY_1}}",     "Normal value"}),
    ASTD({"{{FOO or 'a literal!' or KEY_1}}",   "a literal!"}),
    ASTD({"{{FOO or KEY_1 or 'literal 2!'}}",   "Normal value"}),
    ASTD({"{{ '{{' }}",                         "{{"}),
    ASTD({"{{ '}}' }}",                         "}}"}),
    ASTD({"{{KEY_1}}KEY_2",                     "Normal valueKEY_2"}),
    ASTD({"{{KEY_1 or FOO}} or KEY_1",          "Normal value or KEY_1"})
));
// clang-format on

// TODO: disabled test cases fail because of bugs in the
// librepcb::AttributeSubstitutor!

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
