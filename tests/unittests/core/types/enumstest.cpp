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
#include <librepcb/core/serialization/serialization.h>
#include <librepcb/core/types/enums.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class EnumsTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(EnumsTest, testWireModeSerialization) {
  // Values must remain stable, do not rename!
  const QHash<WireMode, const char*> expected = {
      {WireMode::HV, "h_v"},
      {WireMode::VH, "v_h"},
      {WireMode::Deg9045, "90_45"},
      {WireMode::Deg4590, "45_90"},
      {WireMode::Straight, "straight"},
  };
  QSet<QString> set;
  for (int i = 0; i < static_cast<int>(WireMode::_COUNT); ++i) {
    const WireMode mode = static_cast<WireMode>(i);
    ASSERT_TRUE(expected.contains(mode));

    const QString str = serializeToString(mode);
    EXPECT_FALSE(str.isEmpty());
    EXPECT_FALSE(set.contains(str));
    EXPECT_EQ(expected.value(mode), str);
    set.insert(str);

    const auto parsed = deserialize<WireMode>(str);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(mode, *parsed);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
