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
#include <librepcb/common/version.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class VersionTest : public ::testing::Test
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST(VersionTest, testIsValid)
{
    // valid
    EXPECT_TRUE(Version::isValid("0"));
    EXPECT_TRUE(Version::isValid("05.00000040"));
    EXPECT_TRUE(Version::isValid("00000.00001.00002.00003.00007.00000.00600.00000.08000.20000"));

    // invalid
    EXPECT_FALSE(Version::isValid(""));
    EXPECT_FALSE(Version::isValid("-1"));
    EXPECT_FALSE(Version::isValid("1-0"));
    EXPECT_FALSE(Version::isValid("100000.55"));
    EXPECT_FALSE(Version::isValid("77.-11.9"));
    EXPECT_FALSE(Version::isValid("4.8."));
    EXPECT_FALSE(Version::isValid(".4.8"));
    EXPECT_FALSE(Version::isValid("00000.00001.00002.00003.00007.00000.00600.00000.08000.20000.00030"));
    EXPECT_FALSE(Version::isValid("00000.00001.00002.00003.500007.00000.00600.00000.08000.20000"));
}

TEST(VersionTest, testFromString_failOnEmpty)
{
    EXPECT_THROW(Version::fromString(""), Exception);
}

TEST(VersionTest, testFromString_failOnLeadingDot)
{
    EXPECT_THROW(Version::fromString(".1.2"), Exception);
}

TEST(VersionTest, testFromString_failOnTrailingDot)
{
    EXPECT_THROW(Version::fromString("1."), Exception);
}

TEST(VersionTest, testFromString_failOnNegative)
{
    EXPECT_THROW(Version::fromString("1.-2.3"), Exception);
}

TEST(VersionTest, testFromString_valid)
{
    QString str;
    QVector<uint> numbers;
    for (uint i = 0; i < 10; ++i) {
        numbers.append(i * 10);
        str.append(QString::number(i * 10));
        Version v = Version::fromString(str);
        EXPECT_EQ(numbers, v.getNumbers());
        EXPECT_EQ(str, v.toStr());
        str.append(".");
    }
}

TEST(VersionTest, testTryFromString_nulloptOnEmpty)
{
    EXPECT_EQ(tl::nullopt, Version::tryFromString(""));
}

TEST(VersionTest, testTryFromString_valid)
{
    EXPECT_EQ("1.2", Version::tryFromString("1.2")->toStr());
}

TEST(VersionTest, testCopyConstructor)
{
    Version v1 = Version::fromString("1.2.3");
    Version v2(v1);
    EXPECT_EQ(v1.getNumbers(),      v2.getNumbers());
    EXPECT_EQ(v1.toStr(),           v2.toStr());
    EXPECT_EQ(v1.toPrettyStr(0),    v2.toPrettyStr(0));
    EXPECT_EQ(v1.toComparableStr(), v2.toComparableStr());
}

TEST(VersionTest, testIsPrefixOf)
{
    EXPECT_TRUE(Version::fromString("0")        .isPrefixOf(Version::fromString("0")));
    EXPECT_TRUE(Version::fromString("0.1")      .isPrefixOf(Version::fromString("0.1.0")));
    EXPECT_TRUE(Version::fromString("1.2")      .isPrefixOf(Version::fromString("1.2.0.0.0.1")));
    EXPECT_TRUE(Version::fromString("5.5.5.4")  .isPrefixOf(Version::fromString("5.5.5.4.1")));

    EXPECT_FALSE(Version::fromString("1.2")     .isPrefixOf(Version::fromString("1")));
    EXPECT_FALSE(Version::fromString("0.1")     .isPrefixOf(Version::fromString("0.2")));
    EXPECT_FALSE(Version::fromString("5.5")     .isPrefixOf(Version::fromString("5.4.5")));
}

TEST(VersionTest, testGetNumbers)
{
    EXPECT_EQ(QVector<uint>({0}),          Version::fromString("0").getNumbers());
    EXPECT_EQ(QVector<uint>({5,4,3}),      Version::fromString("5.4.3").getNumbers());
    EXPECT_EQ(QVector<uint>({5,440,0,80}), Version::fromString("005.440.00.080.000").getNumbers());
}

TEST(VersionTest, testToStr)
{
    EXPECT_EQ(QString("0"),                      Version::fromString("0").toStr());
    EXPECT_EQ(QString("5.4.3"),                  Version::fromString("5.4.3").toStr());
    EXPECT_EQ(QString("0.0.6.3.20"),             Version::fromString("0.00.6.003.20.0.0").toStr());
    EXPECT_EQ(QString("5.440.0.80"),             Version::fromString("005.440.00.080.000").toStr());
    EXPECT_EQ(QString("0.1.2.3.7.0.600.0.8000"), Version::fromString("00000.00001.00002.00003.00007.00000.00600.00000.08000.00000").toStr());
}

TEST(VersionTest, testToPrettyStr)
{
    EXPECT_EQ(QString("0"),       Version::fromString("0").toPrettyStr(0, 4));
    EXPECT_EQ(QString("5.0"),     Version::fromString("5").toPrettyStr(2, 3));
    EXPECT_EQ(QString("5.4.3"),   Version::fromString("5.04.3.6.7").toPrettyStr(2, 3));
    EXPECT_EQ(QString("0.0.0.0"), Version::fromString("0").toPrettyStr(4, 4));
}

TEST(VersionTest, testToComparableStr)
{
    EXPECT_EQ(QString("00000.00000.00000.00000.00000.00000.00000.00000.00000.00000"),
              Version::fromString("0").toComparableStr());
    EXPECT_EQ(QString("00001.00000.00000.00000.00000.00000.00000.00000.00000.00000"),
              Version::fromString("1").toComparableStr());
    EXPECT_EQ(QString("00000.00000.00003.00000.00600.00000.00000.00000.00000.00000"),
              Version::fromString("0.0.3.0.600.0").toComparableStr());
}

TEST(VersionTest, testOperatorAssign)
{
    Version v1 = Version::fromString("1.2.3");
    Version v2 = Version::fromString("0.1");
    v2 = v1;
    EXPECT_EQ(v1.getNumbers(),      v2.getNumbers());
    EXPECT_EQ(v1.toStr(),           v2.toStr());
    EXPECT_EQ(v1.toPrettyStr(0),    v2.toPrettyStr(0));
    EXPECT_EQ(v1.toComparableStr(), v2.toComparableStr());
}

TEST(VersionTest, testOperatorGreater)
{
    EXPECT_TRUE(Version::fromString("0.1") > Version::fromString("0.0.9"));
    EXPECT_TRUE(Version::fromString("5.4") > Version::fromString("0.500.0"));
    EXPECT_TRUE(Version::fromString("10.0.0.1") > Version::fromString("10"));

    EXPECT_FALSE(Version::fromString("10") > Version::fromString("10.0.1"));
    EXPECT_FALSE(Version::fromString("0.0.1") > Version::fromString("0.1.0"));
}

TEST(VersionTest, testOperatorLess)
{
    EXPECT_TRUE(Version::fromString("0.0.9") < Version::fromString("0.1"));
    EXPECT_TRUE(Version::fromString("0.500.0") < Version::fromString("5.4"));
    EXPECT_TRUE(Version::fromString("10") < Version::fromString("10.0.0.1"));

    EXPECT_FALSE(Version::fromString("10.0.1") < Version::fromString("10"));
    EXPECT_FALSE(Version::fromString("0.1.0") < Version::fromString("0.0.1"));
}

TEST(VersionTest, testOperatorGreaterEqual)
{
    EXPECT_TRUE(Version::fromString("0.1") >= Version::fromString("0.0.9"));
    EXPECT_TRUE(Version::fromString("5.4") >= Version::fromString("0.500.0"));
    EXPECT_TRUE(Version::fromString("10.0.0.1") >= Version::fromString("10"));
    EXPECT_TRUE(Version::fromString("10.0.0.1") >= Version::fromString("10.0.0.1"));
    EXPECT_TRUE(Version::fromString("5.0.0.5") >= Version::fromString("5.0.0.5.0"));

    EXPECT_FALSE(Version::fromString("10") >= Version::fromString("10.0.1"));
    EXPECT_FALSE(Version::fromString("0.0.1") >= Version::fromString("0.1.0"));
}

TEST(VersionTest, testOperatorLessEqual)
{
    EXPECT_TRUE(Version::fromString("0.0.9") <= Version::fromString("0.1"));
    EXPECT_TRUE(Version::fromString("0.500.0") <= Version::fromString("5.4"));
    EXPECT_TRUE(Version::fromString("10") <= Version::fromString("10.0.0.1"));
    EXPECT_TRUE(Version::fromString("10.0.0.1") <= Version::fromString("10.0.0.1"));
    EXPECT_TRUE(Version::fromString("5.0.0.5") <= Version::fromString("5.0.0.5.0"));

    EXPECT_FALSE(Version::fromString("10.0.1") <= Version::fromString("10"));
    EXPECT_FALSE(Version::fromString("0.1.0") <= Version::fromString("0.0.1"));
}

TEST(VersionTest, testOperatorEqual)
{
    EXPECT_TRUE(Version::fromString("10.0.0.1") == Version::fromString("10.0.0.1"));
    EXPECT_TRUE(Version::fromString("5.0.0.5") == Version::fromString("5.0.0.5.0"));

    EXPECT_FALSE(Version::fromString("10.0.1") == Version::fromString("10"));
    EXPECT_FALSE(Version::fromString("0.1.0") == Version::fromString("0.0.1"));
}

TEST(VersionTest, testOperatorNotEqual)
{
    EXPECT_TRUE(Version::fromString("10.0.0.1") != Version::fromString("10.0.1"));
    EXPECT_TRUE(Version::fromString("5.0.5") != Version::fromString("0.5.0.5"));

    EXPECT_FALSE(Version::fromString("10.0.1") != Version::fromString("10.0.1"));
    EXPECT_FALSE(Version::fromString("0.1.0") != Version::fromString("0.001.0.0.0"));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
