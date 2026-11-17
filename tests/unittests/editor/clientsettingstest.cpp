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
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/types/enums.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/editor/clientsettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ClientSettingsTest : public ::testing::Test {
protected:
  void SetUp() override { QSettings().clear(); }

  std::string readFile() const {
    return FileUtils::readFile(FilePath(QSettings().fileName()))
        .replace("\r\n", "\n")
        .toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ClientSettingsTest, testFileContent) {
  {
    // Store a few values.
    ClientSettings s;
    s.set("angle", Angle::deg45());
    s.set("layer", Layer::topCopper());
    s.set("mode", WireMode::Deg9045);
    s.set("string", QStringLiteral("Hello World!"));
  }  // Destructor syncs the settings to disk.

  {
    // Store a few values in a group.
    ClientSettings s({"foo", "bar"});
    s.set("hello/world", true);
  }  // Destructor syncs the settings to disk.

  const char* expected = R"([General]
angle=45.0
layer=top_cu
mode=90_45
string=Hello World!

[foo]
bar\hello\world=true
)";
  EXPECT_EQ(expected, readFile());
}

TEST_F(ClientSettingsTest, testSetGet) {
  ClientSettings s({"foo", "bar"});
  s.set("string", QStringLiteral("Hello World!"));
  s.set("angle", Angle::deg45());
  s.set("layer", Layer::topCopper());
  s.set("mode", WireMode::Deg9045);
  s.set("nested/bool", true);
  EXPECT_EQ("Hello World!", s.get<QString>("string"));
  EXPECT_EQ(Angle::deg45(), s.get<Angle>("angle"));
  EXPECT_EQ(WireMode::Deg9045, s.get<WireMode>("mode"));
  EXPECT_EQ(&Layer::topCopper(), s.get<const Layer*>("layer"));
  EXPECT_EQ(true, s.get<bool>("nested/bool"));
}

TEST_F(ClientSettingsTest, testGetInexistent) {
  ClientSettings s;
  EXPECT_EQ(std::nullopt, s.get<bool>("does_not_exist"));
  EXPECT_EQ(std::nullopt, s.get<QString>("does_not_exist"));
  EXPECT_EQ(std::nullopt, s.get<Angle>("does_not_exist"));
}

TEST_F(ClientSettingsTest, testGetInvalid) {
  ClientSettings s;
  s.set("bool", QStringLiteral("invalid"));
  s.set("angle", QStringLiteral("invalid"));
  s.set("mode", QStringLiteral("invalid"));
  s.set("layer", QStringLiteral(""));
  EXPECT_EQ(std::nullopt, s.get<bool>("bool"));
  EXPECT_EQ(std::nullopt, s.get<Angle>("angle"));
  EXPECT_EQ(std::nullopt, s.get<WireMode>("mode"));
  EXPECT_EQ(std::nullopt, s.get<const Layer*>("layer"));
}

TEST_F(ClientSettingsTest, testGetAngle) {
  ClientSettings s;
  s.set("angle", Angle::deg45());

  // min
  EXPECT_EQ(Angle::deg45(), s.get("angle", Angle::deg45(), std::nullopt));
  EXPECT_EQ(std::nullopt, s.get("angle", Angle::deg90(), std::nullopt));

  // max
  EXPECT_EQ(Angle::deg45(), s.get("angle", std::nullopt, Angle::deg45()));
  EXPECT_EQ(std::nullopt, s.get("angle", std::nullopt, Angle::deg0()));

  // modulo
  EXPECT_EQ(Angle::deg45(),
            s.get("angle", std::nullopt, std::nullopt, Angle::deg45()));
  EXPECT_EQ(std::nullopt,
            s.get("angle", std::nullopt, std::nullopt, Angle::deg90()));
}

TEST_F(ClientSettingsTest, testGetLayer) {
  ClientSettings s;
  s.set("layer", Layer::topCopper());

  QSet<const Layer*> set = {&Layer::topCopper(), &Layer::botCopper()};
  EXPECT_EQ(&Layer::topCopper(), s.get("layer", set));
  set = {&Layer::botCopper()};
  EXPECT_EQ(std::nullopt, s.get("layer", set));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
