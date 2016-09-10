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
#include <librepcbcommon/uuid.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Data Type
 ****************************************************************************************/

typedef struct {
    bool valid;
    QString uuid;
} UuidTestData;

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class UuidTest : public ::testing::TestWithParam<UuidTestData>
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST(UuidTest, testDefaultConstructor)
{
    Uuid uuid;
    EXPECT_TRUE(uuid.isNull());
    EXPECT_TRUE(uuid.toStr().isNull());
}

TEST_P(UuidTest, testCopyConstructor)
{
    const UuidTestData& data = GetParam();

    Uuid source(data.uuid);
    Uuid copy(source);
    EXPECT_EQ(source.isNull(), copy.isNull());
    EXPECT_EQ(source.toStr(), copy.toStr());
}

TEST_P(UuidTest, testStringConstructor)
{
    const UuidTestData& data = GetParam();

    Uuid uuid(data.uuid);
    EXPECT_EQ(data.valid, !uuid.isNull());
    if (data.valid) {
        EXPECT_EQ(data.uuid.toLower(), uuid.toStr());
    } else {
        EXPECT_EQ(QString(), uuid.toStr());
    }
}

TEST_P(UuidTest, testIsNullAndToStrAndSetUuid)
{
    const UuidTestData& data = GetParam();

    {
        Uuid uuid;
        EXPECT_EQ(data.valid, uuid.setUuid(data.uuid));
        EXPECT_EQ(data.valid, !uuid.isNull());
        EXPECT_EQ(data.valid, !uuid.toStr().isNull());
        EXPECT_EQ(uuid.toStr().toLower(), uuid.toStr());
        if (data.valid) {
            EXPECT_EQ(36, uuid.toStr().length());
            EXPECT_EQ(data.uuid.toLower(), uuid.toStr());
        } else {
            EXPECT_EQ(0, uuid.toStr().length());
            EXPECT_EQ(QString(), uuid.toStr());
        }
    }

    {
        Uuid uuid("d2c30518-5cd1-4ce9-a569-44f783a3f66a"); // valid UUID
        EXPECT_EQ(data.valid, uuid.setUuid(data.uuid));
        EXPECT_EQ(data.valid, !uuid.isNull());
        EXPECT_EQ(data.valid, !uuid.toStr().isNull());
        EXPECT_EQ(uuid.toStr().toLower(), uuid.toStr());
        if (data.valid) {
            EXPECT_EQ(36, uuid.toStr().length());
            EXPECT_EQ(data.uuid.toLower(), uuid.toStr());
        } else {
            EXPECT_EQ(0, uuid.toStr().length());
            EXPECT_EQ(QString(), uuid.toStr());
        }
    }
}

TEST_P(UuidTest, testOperatorAssign)
{
    const UuidTestData& data = GetParam();

    Uuid source(data.uuid);
    Uuid destination("");
    destination = source;
    EXPECT_EQ(source.isNull(), destination.isNull());
    EXPECT_EQ(source.toStr(), destination.toStr());
}

TEST_P(UuidTest, testOperatorEquals)
{
    const UuidTestData& data = GetParam();

    Uuid uuid1(data.uuid);
    Uuid uuid2("d2c30518-5cd1-4ce9-a569-44f783a3f66a"); // valid UUID
    EXPECT_FALSE(uuid2 == uuid1);
    EXPECT_FALSE(uuid1 == uuid2);
    uuid2 = uuid1;
    if (data.valid) {
        EXPECT_TRUE(uuid2 == uuid1);
        EXPECT_TRUE(uuid1 == uuid2);
        EXPECT_EQ(uuid2.toStr() == uuid1.toStr(), uuid2 == uuid1);
        EXPECT_EQ(uuid1.toStr() == uuid2.toStr(), uuid1 == uuid2);
    } else {
        EXPECT_FALSE(uuid2 == uuid1);
        EXPECT_FALSE(uuid1 == uuid2);
    }
}

TEST_P(UuidTest, testOperatorNotEquals)
{
    const UuidTestData& data = GetParam();

    Uuid uuid1(data.uuid);
    Uuid uuid2("d2c30518-5cd1-4ce9-a569-44f783a3f66a"); // valid UUID
    EXPECT_TRUE(uuid2 != uuid1);
    EXPECT_TRUE(uuid1 != uuid2);
    uuid2 = uuid1;
    if (data.valid) {
        EXPECT_FALSE(uuid2 != uuid1);
        EXPECT_FALSE(uuid1 != uuid2);
        EXPECT_EQ(uuid2.toStr() != uuid1.toStr(), uuid2 != uuid1);
        EXPECT_EQ(uuid1.toStr() != uuid2.toStr(), uuid1 != uuid2);
    } else {
        EXPECT_TRUE(uuid2 != uuid1);
        EXPECT_TRUE(uuid1 != uuid2);
    }
}

TEST_P(UuidTest, testOperatorComparisons)
{
    const UuidTestData& data = GetParam();

    Uuid uuid1(data.uuid);
    Uuid uuid2("74CA6127-E785-4355-8580-1CED4F0A0E9E"); // valid UUID
    if (data.valid) {
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
    } else {
        EXPECT_FALSE(uuid2 < uuid1);
        EXPECT_FALSE(uuid1 < uuid2);
        EXPECT_FALSE(uuid2 > uuid1);
        EXPECT_FALSE(uuid1 > uuid2);
        EXPECT_FALSE(uuid2 <= uuid1);
        EXPECT_FALSE(uuid1 <= uuid2);
        EXPECT_FALSE(uuid2 >= uuid1);
        EXPECT_FALSE(uuid1 >= uuid2);
    }
}

TEST(UuidTest, testCreateRandom)
{
    for (int i = 0; i < 1000; i++) {
        Uuid uuid = Uuid::createRandom();
        ASSERT_FALSE(uuid.isNull());
        ASSERT_FALSE(uuid.toStr().isEmpty());
        ASSERT_EQ(QUuid::DCE, QUuid(uuid.toStr()).variant());
        ASSERT_EQ(QUuid::Random, QUuid(uuid.toStr()).version());
    }
}

/*****************************************************************************************
 *  Test Data
 ****************************************************************************************/

// Test UUIDs are generated with:
//  - https://www.uuidgenerator.net
//  - https://uuidgenerator.org/
//  - https://www.famkruithof.net/uuid/uuidgen
//  - http://www.freecodeformat.com/uuid-guid.php
//  - https://de.wikipedia.org/wiki/Universally_Unique_Identifier
INSTANTIATE_TEST_CASE_P(UuidTest, UuidTest, ::testing::Values(
    // DCE Version 4 (random, the only accepted UUID type for us)
    UuidTestData({true , "bdf7bea5-b88e-41b2-be85-c1604e8ddfca"  }),
    UuidTestData({true , "587539af-1c39-40ed-9bdd-2ca2e6aeb18d"  }),
    UuidTestData({true , "27556d27-fe33-4334-a8ee-b05b402a21d6"  }),
    UuidTestData({true , "91172d44-bdcc-41b2-8e07-4f8cf44eb108"  }),
    UuidTestData({true , "ecb3a5fe-1cbc-4a1b-bf8f-5d6e26deaee1"  }),
    UuidTestData({true , "908f9c33-40be-46aa-97b4-be2cd7477881"  }),
    UuidTestData({true , "74CA6127-E785-4355-8580-1CED4F0A0E9E"  }),
    UuidTestData({true , "568EB40D-CD69-47A5-8932-4F5CC4B2D3FA"  }),
    UuidTestData({true , "29401DCB-6CB6-47A1-8F7D-72DD7F9F4939"  }),
    UuidTestData({true , "E367D539-3163-4530-AB47-3B4CB2DF2A40"  }),
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
    UuidTestData({false, "C56A4180-65AA-42EC-A945-5FD21DEC"      }),    // too short
    UuidTestData({false, "bdf7bea5-b88e-41b2-be85-c1604e8ddfca " }),    // too long
    UuidTestData({false, " bdf7bea5-b88e-41b2-be85-c1604e8ddfca" }),    // too long
    UuidTestData({false, "bdf7bea5b88e41b2be85c1604e8ddfca"      }),    // missing '-'
    UuidTestData({false, "{bdf7bea5-b88e-41b2-be85-c1604e8ddfca}"}),    // '{', '}'
    UuidTestData({false, "bdf7bea5-b88g-41b2-be85-c1604e8ddfca"  }),    // 'g'
    UuidTestData({false, "bdf7bea5_b88e_41b2_be85_c1604e8ddfca"  }),    // '_'
    UuidTestData({false, "bdf7bea5 b88e 41b2 be85 c1604e8ddfca"  })     // spaces
));

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
