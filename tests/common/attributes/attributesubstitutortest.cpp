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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <gtest/gtest.h>
#include "attributeproviderdummy.h"
#include <librepcb/common/attributes/attributesubstitutor.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Data Type
 ****************************************************************************************/

typedef struct {
    const QString input;
    const QString output;
} AttributeSubstitutorTestData;

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class AttributeSubstitutorTest : public ::testing::TestWithParam<AttributeSubstitutorTestData>
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_P(AttributeSubstitutorTest, testData)
{
    const AttributeSubstitutorTestData& data = GetParam();

    AttributeProviderDummy ap;
    QString output = AttributeSubstitutor::substitute(data.input, &ap);
    EXPECT_EQ(data.output, output) << "Actual value: '" << qPrintable(output) << "'";
}

/*****************************************************************************************
 *  Test Data
 ****************************************************************************************/

using ASTD = AttributeSubstitutorTestData;

INSTANTIATE_TEST_CASE_P(AttributeSubstitutorTest, AttributeSubstitutorTest, ::testing::Values(
    ASTD({"",                               ""}),
    ASTD({"#NONEXISTENT",                   ""}),
    ASTD({"#KEY",                           ""}),
    ASTD({"#KEY_1",                         "Normal value"}),
    //ASTD({"#KEY_1 #KEY_1",                  "Normal value Normal value"}),
    ASTD({"##escaped##",                    "#escaped#"}),
    ASTD({"#KEY_2",                         "Value with #escaping#"}),
    ASTD({"#KEY_3",                         "Recursive  value"}),
    ASTD({"#KEY_4",                         "Recursive Normal value value"}),
    ASTD({"#KEY_5",                         "Recursive Recursive Normal value value value"}),
    ASTD({"#KEY_6",                         "Endless Endless  part 2 part 1"}),
    ASTD({"#KEY_7",                         "Endless Endless  part 1 part 2"}),
    ASTD({"Foo ##KEY_7 ###KEY_7 #KEYY",     "Foo #KEY_7 #Endless Endless  part 1 part 2 "}),
    ASTD({"#KEY_3 foo# # KEY_5## #KEY",     "Recursive  value foo# # KEY_5# "}),
    ASTD({"#KEY_1 #KEY_2|KEY_3|KEY_4 foo",  "Normal value Value with #escaping# foo"}),
    //ASTD({"#KEY_8|KEY_1",                   "Normal value"}),
    //ASTD({"#KEY|KEY_4|KEY_3 #KEY_1",        "Recursive Normal value value Normal value"}),
    //ASTD({"###KEY_1 #FOO|KEY|KEY_5## foo",  "#Normal value Recursive Recursive Normal value value value# foo"}),
    ASTD({"#FOO|BAR|BAR|FOO",               ""}),
    ASTD({"#FOO|BAR|BAR|FOO||",             ""}),
    ASTD({"#KEY_1||KEY_2",                  "Normal valueKEY_2"}),
    ASTD({"#KEY_1|FOO|||KEY_1",             "Normal value|KEY_1"})
));

// TODO: disabled test cases fail because of bugs in the librepcb::AttributeSubstitutor!

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
