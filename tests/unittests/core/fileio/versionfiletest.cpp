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
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/versionfile.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class VersionFileTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(VersionFileTest, testGetVersion) {
  Version v = Version::fromString("1.2.3");
  VersionFile p{v};
  EXPECT_EQ(v, p.getVersion());
}

TEST_F(VersionFileTest, testSetVersion) {
  Version v1 = Version::fromString("1.2.3");
  Version v2 = Version::fromString("1.5.3");
  VersionFile p{v1};
  p.setVersion(v2);
  EXPECT_EQ(v2, p.getVersion());
}

TEST_F(VersionFileTest, testToByteArray) {
  VersionFile p{Version::fromString("1.2.3")};
  auto result = p.toByteArray();
  EXPECT_EQ("1.2.3\n", result);
}

TEST_F(VersionFileTest, testFromByteArrayNormal) {
  VersionFile p = VersionFile::fromByteArray("1.2.3\n");
  EXPECT_EQ("1.2.3", p.getVersion().toStr());
}

TEST_F(VersionFileTest, testFromByteArrayNoEol) {
  VersionFile p = VersionFile::fromByteArray("1.2.3");
  EXPECT_EQ("1.2.3", p.getVersion().toStr());
}

TEST_F(VersionFileTest, testFromByteArrayMultiline) {
  VersionFile p = VersionFile::fromByteArray("1.2.3\nsomecomment\n");
  EXPECT_EQ("1.2.3", p.getVersion().toStr());
}

TEST_F(VersionFileTest, testFromByteArrayWrong) {
  EXPECT_THROW(VersionFile::fromByteArray("dead"), Exception);
}

TEST_F(VersionFileTest, testFromByteArrayEmpty) {
  EXPECT_THROW(VersionFile::fromByteArray(""), Exception);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
