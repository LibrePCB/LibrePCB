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
#include <librepcb/automation/toolhelpers.h>
#include <librepcb/core/serialization/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ToolHelpersTest : public ::testing::Test {
protected:
  class Message final : public RuleCheckMessage {
  public:
    Message(const QString& msg, const QString& description,
            const QVector<Path>& locations)
      : RuleCheckMessage(Severity::Warning, msg, description, "test",
                         locations) {
      mApproval->appendChild("msg", msg);
    }
  };

  static RuleCheckMessageList messages(
      std::initializer_list<std::shared_ptr<const RuleCheckMessage>> list) {
    return RuleCheckMessageList(list);
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ToolHelpersTest, testMessageLocationsAreAreas) {
  const QJsonObject result =
      toJson(messages({std::make_shared<Message>(
                 "Two places", "Description",
                 QVector<Path>{
                     Path::rect(Point::fromMm(10, 10), Point::fromMm(12, 14)),
                     Path::obround(Point::fromMm(0, 5), Point::fromMm(20, 5),
                                   PositiveLength(1000000))})}),
             {});
  const QJsonArray locations = result.value("messages")
                                   .toArray()
                                   .at(0)
                                   .toObject()
                                   .value("locations")
                                   .toArray();
  ASSERT_EQ(2, locations.count());
  EXPECT_EQ(QJsonArray({11, 12}),
            locations.at(0).toObject().value("center").toArray());
  EXPECT_EQ(QJsonArray({2, 4}),
            locations.at(0).toObject().value("size").toArray());
  // A trace is marked along its whole length, its center is in the middle.
  EXPECT_EQ(QJsonArray({10, 5}),
            locations.at(1).toObject().value("center").toArray());
  EXPECT_EQ(QJsonArray({21, 1}),
            locations.at(1).toObject().value("size").toArray());
}

TEST_F(ToolHelpersTest, testManyLocationsAreLimited) {
  QVector<Path> locations;
  for (int i = 0; i < 25; ++i) {
    locations.append(Path::rect(Point::fromMm(i, 0), Point::fromMm(i + 1, 1)));
  }
  const QJsonObject msg =
      toJson(messages({std::make_shared<Message>("Many", "D", locations)}), {})
          .value("messages")
          .toArray()
          .at(0)
          .toObject();
  EXPECT_EQ(10, msg.value("locations").toArray().count());
  EXPECT_EQ(15, msg.value("moreLocations").toInt());
}

TEST_F(ToolHelpersTest, testEachDescriptionIsGivenOnce) {
  const QJsonArray array =
      toJson(
          messages({std::make_shared<Message>("A", "Same", QVector<Path>{}),
                    std::make_shared<Message>("B", "Same", QVector<Path>{}),
                    std::make_shared<Message>("C", "Other", QVector<Path>{})}),
          {})
          .value("messages")
          .toArray();
  ASSERT_EQ(3, array.count());
  EXPECT_EQ("Same", array.at(0).toObject().value("description").toString());
  EXPECT_FALSE(array.at(1).toObject().contains("description"));
  EXPECT_EQ("Other", array.at(2).toObject().value("description").toString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace automation
}  // namespace librepcb
