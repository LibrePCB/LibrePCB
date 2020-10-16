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
#include <librepcb/common/signalrole.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SignalRoleTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(SignalRoleTest, testSerialize) {
  EXPECT_EQ("opendrain\n", serialize(SignalRole::opendrain()).toByteArray());
}

TEST(SignalRoleTest, testDeserialize) {
  SExpression sexpr = SExpression::createString("opendrain");
  EXPECT_EQ(SignalRole::opendrain(), deserialize<SignalRole>(sexpr, false));
}

TEST(SignalRoleTest, testDeserializeEmpty) {
  SExpression sexpr = SExpression::createString("");
  EXPECT_THROW(deserialize<SignalRole>(sexpr, false), RuntimeError);
}

TEST(SignalRoleTest, testDeserializeInvalid) {
  SExpression sexpr = SExpression::createString("foo");
  EXPECT_THROW(deserialize<SignalRole>(sexpr, false), RuntimeError);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
