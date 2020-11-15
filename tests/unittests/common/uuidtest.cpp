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
#include <librepcb/common/application.h>
#include <librepcb/common/uuid.h>

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
  bool valid;
  QString uuid;
} UuidTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class UuidTest : public ::testing::TestWithParam<UuidTestData> {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(UuidTest, testCopyConstructor) {
  const UuidTestData& data = GetParam();

  if (data.valid) {
    Uuid source = Uuid::fromString(data.uuid);
    Uuid copy(source);
    EXPECT_TRUE(copy == source);
    EXPECT_EQ(source.toStr(), copy.toStr());
  }
}

TEST_P(UuidTest, testToStr) {
  const UuidTestData& data = GetParam();

  if (data.valid) {
    Uuid uuid = Uuid::fromString(data.uuid);
    EXPECT_EQ(data.uuid, uuid.toStr());
    EXPECT_EQ(36, uuid.toStr().length());
  }
}

TEST_P(UuidTest, testOperatorAssign) {
  const UuidTestData& data = GetParam();

  if (data.valid) {
    Uuid source = Uuid::fromString(data.uuid);
    Uuid destination =
        Uuid::fromString("d2c30518-5cd1-4ce9-a569-44f783a3f66a");  // valid UUID
    EXPECT_NE(source.toStr(), destination.toStr());
    destination = source;
    EXPECT_EQ(source.toStr(), destination.toStr());
  }
}

TEST_P(UuidTest, testOperatorEquals) {
  const UuidTestData& data = GetParam();

  if (data.valid) {
    Uuid uuid1 = Uuid::fromString(data.uuid);
    Uuid uuid2 =
        Uuid::fromString("d2c30518-5cd1-4ce9-a569-44f783a3f66a");  // valid UUID
    EXPECT_FALSE(uuid2 == uuid1);
    EXPECT_FALSE(uuid1 == uuid2);
    uuid2 = uuid1;
    EXPECT_TRUE(uuid2 == uuid1);
    EXPECT_TRUE(uuid1 == uuid2);
  }
}

TEST_P(UuidTest, testOperatorNotEquals) {
  const UuidTestData& data = GetParam();

  if (data.valid) {
    Uuid uuid1 = Uuid::fromString(data.uuid);
    Uuid uuid2 =
        Uuid::fromString("d2c30518-5cd1-4ce9-a569-44f783a3f66a");  // valid UUID
    EXPECT_TRUE(uuid2 != uuid1);
    EXPECT_TRUE(uuid1 != uuid2);
    uuid2 = uuid1;
    EXPECT_FALSE(uuid2 != uuid1);
    EXPECT_FALSE(uuid1 != uuid2);
  }
}

TEST_P(UuidTest, testOperatorComparisons) {
  const UuidTestData& data = GetParam();

  if (data.valid) {
    Uuid uuid1 = Uuid::fromString(data.uuid);
    Uuid uuid2 =
        Uuid::fromString("d2c30518-5cd1-4ce9-a569-44f783a3f66a");  // valid UUID
    if (uuid1.toStr() == uuid2.toStr()) {
      EXPECT_FALSE((uuid2 < uuid1) || (uuid2 > uuid1));
      EXPECT_FALSE((uuid1 < uuid2) || (uuid1 > uuid2));
      EXPECT_TRUE((uuid2 <= uuid1) && (uuid2 >= uuid1));
      EXPECT_TRUE((uuid1 <= uuid2) && (uuid1 >= uuid2));
    } else {
      EXPECT_TRUE((uuid2 < uuid1) != (uuid2 > uuid1));
      EXPECT_TRUE((uuid1 < uuid2) != (uuid1 > uuid2));
      EXPECT_TRUE((uuid2 <= uuid1) != (uuid2 >= uuid1));
      EXPECT_TRUE((uuid1 <= uuid2) != (uuid1 >= uuid2));
    }
    EXPECT_EQ(uuid2.toStr() < uuid1.toStr(), uuid2 < uuid1);
    EXPECT_EQ(uuid1.toStr() < uuid2.toStr(), uuid1 < uuid2);
    EXPECT_EQ(uuid2.toStr() > uuid1.toStr(), uuid2 > uuid1);
    EXPECT_EQ(uuid1.toStr() > uuid2.toStr(), uuid1 > uuid2);
    EXPECT_EQ(uuid2.toStr() <= uuid1.toStr(), uuid2 <= uuid1);
    EXPECT_EQ(uuid1.toStr() <= uuid2.toStr(), uuid1 <= uuid2);
    EXPECT_EQ(uuid2.toStr() >= uuid1.toStr(), uuid2 >= uuid1);
    EXPECT_EQ(uuid1.toStr() >= uuid2.toStr(), uuid1 >= uuid2);
  }
}

TEST(UuidTest, testCreateRandom) {
  for (int i = 0; i < 1000; i++) {
    Uuid uuid = Uuid::createRandom();
    EXPECT_FALSE(uuid.toStr().isEmpty());
    EXPECT_EQ(QUuid::DCE, QUuid(uuid.toStr()).variant());
    EXPECT_EQ(QUuid::Random, QUuid(uuid.toStr()).version());
  }
}

TEST_P(UuidTest, testIsValid) {
  const UuidTestData& data = GetParam();
  EXPECT_EQ(data.valid, Uuid::isValid(data.uuid));
}

TEST_P(UuidTest, testFromString) {
  const UuidTestData& data = GetParam();
  if (data.valid) {
    EXPECT_EQ(data.uuid, Uuid::fromString(data.uuid).toStr());
  } else {
    EXPECT_THROW(Uuid::fromString(data.uuid), Exception);
  }
}

TEST_P(UuidTest, testTryFromString) {
  const UuidTestData& data = GetParam();
  tl::optional<Uuid> uuid = Uuid::tryFromString(data.uuid);
  if (data.valid) {
    EXPECT_TRUE(uuid);
    EXPECT_EQ(data.uuid, uuid->toStr());
  } else {
    EXPECT_FALSE(uuid);
    EXPECT_EQ(tl::nullopt, uuid);
  }
}

TEST_P(UuidTest, testSerialize) {
  const UuidTestData& data = GetParam();
  if (data.valid) {
    Uuid uuid = Uuid::fromString(data.uuid);
    EXPECT_EQ(data.uuid, serialize(uuid).getValue());
    EXPECT_EQ(data.uuid, serialize(tl::make_optional(uuid)).getValue());
  }
}

TEST_P(UuidTest, testDeserializeV01) {
  const UuidTestData& data = GetParam();
  SExpression sexpr = SExpression::createToken(data.uuid);
  if (data.valid) {
    EXPECT_EQ(data.uuid,
              deserialize<Uuid>(sexpr, Version::fromString("0.1")).toStr());
    EXPECT_EQ(data.uuid,
              deserialize<tl::optional<Uuid>>(sexpr, Version::fromString("0.1"))
                  ->toStr());
  } else {
    EXPECT_THROW(deserialize<Uuid>(sexpr, Version::fromString("0.1")),
                 Exception);
    EXPECT_THROW(
        deserialize<tl::optional<Uuid>>(sexpr, Version::fromString("0.1")),
        Exception);
  }
}

TEST_P(UuidTest, testDeserializeCurrentVersion) {
  const UuidTestData& data = GetParam();
  SExpression sexpr = SExpression::createToken(data.uuid);
  if (data.valid) {
    EXPECT_EQ(data.uuid,
              deserialize<Uuid>(sexpr, qApp->getFileFormatVersion()).toStr());
    EXPECT_EQ(
        data.uuid,
        deserialize<tl::optional<Uuid>>(sexpr, qApp->getFileFormatVersion())
            ->toStr());
  } else {
    EXPECT_THROW(deserialize<Uuid>(sexpr, qApp->getFileFormatVersion()),
                 Exception);
    EXPECT_THROW(
        deserialize<tl::optional<Uuid>>(sexpr, qApp->getFileFormatVersion()),
        Exception);
  }
}

TEST(UuidTest, testSerializeOptional) {
  tl::optional<Uuid> uuid = tl::nullopt;
  EXPECT_EQ("none", serialize(uuid).getValue());
}

TEST(UuidTest, testDeserializeOptionalV01) {
  // Attention: Do NOT modify this string! It represents the freezed(!) file
  // format V0.1 and even current versions of LibrePCB must be able to load it!
  SExpression sexpr = SExpression::createToken("none");
  EXPECT_EQ(tl::nullopt,
            deserialize<tl::optional<Uuid>>(sexpr, Version::fromString("0.1")));
}

TEST(UuidTest, testDeserializeOptionalCurrentVersion) {
  SExpression sexpr = SExpression::createToken("none");
  EXPECT_EQ(
      tl::nullopt,
      deserialize<tl::optional<Uuid>>(sexpr, qApp->getFileFormatVersion()));
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// Test UUIDs are generated with:
//  - https://www.uuidgenerator.net
//  - https://uuidgenerator.org/
//  - https://www.famkruithof.net/uuid/uuidgen
//  - http://www.freecodeformat.com/uuid-guid.php
//  - https://de.wikipedia.org/wiki/Universally_Unique_Identifier
//
// clang-format off
INSTANTIATE_TEST_SUITE_P(UuidTest, UuidTest, ::testing::Values(
    // DCE Version 4 (random, the only accepted UUID type for us)
    UuidTestData({true , "bdf7bea5-b88e-41b2-be85-c1604e8ddfca"  }),
    UuidTestData({true , "587539af-1c39-40ed-9bdd-2ca2e6aeb18d"  }),
    UuidTestData({true , "27556d27-fe33-4334-a8ee-b05b402a21d6"  }),
    UuidTestData({true , "91172d44-bdcc-41b2-8e07-4f8cf44eb108"  }),
    UuidTestData({true , "ecb3a5fe-1cbc-4a1b-bf8f-5d6e26deaee1"  }),
    UuidTestData({true , "908f9c33-40be-46aa-97b4-be2cd7477881"  }),
    UuidTestData({true , "74ca6127-e785-4355-8580-1ced4f0a0e9e"  }),
    UuidTestData({true , "568eb40d-cd69-47a5-8932-4f5cc4b2d3fa"  }),
    UuidTestData({true , "29401dcb-6cb6-47a1-8f7d-72dd7f9f4939"  }),
    UuidTestData({true , "e367d539-3163-4530-ab47-3b4cb2df2a40"  }),
    UuidTestData({true , "00000000-0000-4001-8000-000000000000"  }),
    // DCE Version 1 (time based)
    UuidTestData({false, "15edb784-76df-11e6-8b77-86f30ca893d3"  }),
    UuidTestData({false, "232872b8-76df-11e6-8b77-86f30ca893d3"  }),
    UuidTestData({false, "1d5a3bd6-76e0-11e6-b25e-0401beb96201"  }),
    UuidTestData({false, "F0CDE9F0-76DF-11E6-BDF4-0800200C9A66"  }),
    UuidTestData({false, "EA9A1590-76DF-11E6-BDF4-0800200C9A66"  }),
    // DCE Version 3 (name based, md5)
    UuidTestData({false, "1a32cba8-79ba-3f01-bd8a-46c5ae17ccd8"  }),
    UuidTestData({false, "BBCB4DF8-95FB-38E8-A398-187EA35A1655"  }),
    // DCE Version 5 (name based, sha1)
    UuidTestData({false, "74738ff5-5367-5958-9aee-98fffdcd1876"  }),
    // Microsoft GUID
    UuidTestData({false, "00000000-0000-0000-C000-000000000046"  }),
    // NULL UUID
    UuidTestData({false, "00000000-0000-0000-0000-000000000000"  }),
    // Invalid UUIDs
    UuidTestData({false, ""                                      }),    // empty
    UuidTestData({false, "                                    "  }),    // empty
    UuidTestData({false, QString()}),                                   // null
    UuidTestData({false, "\nbdf7bea5-b88e-41b2-be85-c1604e8ddfca"}),    // newline
    UuidTestData({false, "bdf7bea5-b88e-41b2-be85-c1604e8ddfca\n"}),    // newline
    UuidTestData({false, "74CA6127-E785-4355-8580-1CED4F0A0E9E"  }),    // uppercase
    UuidTestData({false, "568EB40D-CD69-47A5-8932-4F5CC4B2D3FA"  }),    // uppercase
    UuidTestData({false, "29401DCB-6CB6-47A1-8F7D-72DD7F9F4939"  }),    // uppercase
    UuidTestData({false, "E367D539-3163-4530-AB47-3B4CB2DF2A40"  }),    // uppercase
    UuidTestData({false, "C56A4180-65AA-42EC-A945-5FD21DEC"      }),    // too short
    UuidTestData({false, "bdf7bea5-b88e-41b2-be85-c1604e8ddfca " }),    // too long
    UuidTestData({false, " bdf7bea5-b88e-41b2-be85-c1604e8ddfca" }),    // too long
    UuidTestData({false, "bdf7bea5b88e41b2be85c1604e8ddfca"      }),    // missing '-'
    UuidTestData({false, "{bdf7bea5-b88e-41b2-be85-c1604e8ddfca}"}),    // '{', '}'
    UuidTestData({false, "bdf7bea5-b88g-41b2-be85-c1604e8ddfca"  }),    // 'g'
    UuidTestData({false, "bdf7bea5_b88e_41b2_be85_c1604e8ddfca"  }),    // '_'
    UuidTestData({false, "bdf7bea5 b88e 41b2 be85 c1604e8ddfca"  })     // spaces
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
