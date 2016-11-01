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

TEST(VersionTest, testDefaultConstructor)
{
    Version v;
    EXPECT_FALSE(v.isValid());
    EXPECT_EQ(0, v.getNumbers().count());
    EXPECT_TRUE(v.toStr().isEmpty());
    EXPECT_TRUE(v.toPrettyStr(0).isEmpty());
    EXPECT_TRUE(v.toComparableStr().isEmpty());
}

TEST(VersionTest, testCopyConstructor)
{
    Version v1("1.2.3");
    Version v2(v1);
    EXPECT_EQ(v1.isValid(),         v2.isValid());
    EXPECT_EQ(v1.getNumbers(),      v2.getNumbers());
    EXPECT_EQ(v1.toStr(),           v2.toStr());
    EXPECT_EQ(v1.toPrettyStr(0),    v2.toPrettyStr(0));
    EXPECT_EQ(v1.toComparableStr(), v2.toComparableStr());
}

TEST(VersionTest, testConstructorWithString)
{
    Version v("0.1.2.3.0");
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(4, v.getNumbers().count());
    EXPECT_EQ(QList<int>({0,1,2,3}), v.getNumbers());
    EXPECT_EQ("0.1.2.3", v.toStr());
    EXPECT_EQ("0.1.2.3", v.toPrettyStr(0));
    EXPECT_EQ("00000.00001.00002.00003.00000.00000.00000.00000.00000.00000", v.toComparableStr());
}

TEST(VersionTest, testIsValid)
{
    // valid
    EXPECT_TRUE(Version("0").isValid());
    EXPECT_TRUE(Version("05.00000040").isValid());
    EXPECT_TRUE(Version("00000.00001.00002.00003.00007.00000.00600.00000.08000.20000").isValid());

    // invalid
    EXPECT_FALSE(Version("").isValid());
    EXPECT_FALSE(Version("-1").isValid());
    EXPECT_FALSE(Version("1-0").isValid());
    EXPECT_FALSE(Version("100000.55").isValid());
    EXPECT_FALSE(Version("77.-11.9").isValid());
    EXPECT_FALSE(Version("4.8.").isValid());
    EXPECT_FALSE(Version(".4.8").isValid());
    EXPECT_FALSE(Version("00000.00001.00002.00003.00007.00000.00600.00000.08000.20000.00030").isValid());
    EXPECT_FALSE(Version("00000.00001.00002.00003.500007.00000.00600.00000.08000.20000").isValid());
}

TEST(VersionTest, testIsPrefixOf)
{
    EXPECT_TRUE(Version("0")        .isPrefixOf(Version("0")));
    EXPECT_TRUE(Version("0.1")      .isPrefixOf(Version("0.1.0")));
    EXPECT_TRUE(Version("1.2")      .isPrefixOf(Version("1.2.0.0.0.1")));
    EXPECT_TRUE(Version("5.5.5.4")  .isPrefixOf(Version("5.5.5.4.1")));

    EXPECT_FALSE(Version("")        .isPrefixOf(Version("0")));
    EXPECT_FALSE(Version("0")       .isPrefixOf(Version("")));
    EXPECT_FALSE(Version("1.2")     .isPrefixOf(Version("1")));
    EXPECT_FALSE(Version("0.1")     .isPrefixOf(Version("0.2")));
    EXPECT_FALSE(Version("5.5")     .isPrefixOf(Version("5.4.5")));
}

TEST(VersionTest, testGetNumbers)
{
    EXPECT_EQ(QList<int>({}),           Version("").getNumbers());
    EXPECT_EQ(QList<int>({0}),          Version("0").getNumbers());
    EXPECT_EQ(QList<int>({5,4,3}),      Version("5.4.3").getNumbers());
    EXPECT_EQ(QList<int>({5,440,0,80}), Version("005.440.00.080.000").getNumbers());
}

TEST(VersionTest, testToStr)
{
    EXPECT_EQ(QString(),                            Version("-1").toStr());
    EXPECT_EQ(QString("0"),                         Version("0").toStr());
    EXPECT_EQ(QString("5.4.3"),                     Version("5.4.3").toStr());
    EXPECT_EQ(QString("0.0.6.3.20"),                Version("0.00.6.003.20.0.0").toStr());
    EXPECT_EQ(QString("5.440.0.80"),                Version("005.440.00.080.000").toStr());
    EXPECT_EQ(QString("0.1.2.3.7.0.600.0.8000"),    Version("00000.00001.00002.00003.00007.00000.00600.00000.08000.00000").toStr());
}

TEST(VersionTest, testToPrettyStr)
{
    EXPECT_EQ(QString(),                            Version("-1").toPrettyStr(0, 10));
    EXPECT_EQ(QString("0"),                         Version("0").toPrettyStr(0, 4));
    EXPECT_EQ(QString("5.0"),                       Version("5").toPrettyStr(2, 3));
    EXPECT_EQ(QString("5.4.3"),                     Version("5.04.3.6.7").toPrettyStr(2, 3));
    EXPECT_EQ(QString("0.0.0.0"),                   Version("0").toPrettyStr(4, 4));
}

TEST(VersionTest, testToComparableStr)
{
    EXPECT_EQ(QString(),
              Version("-1").toComparableStr());
    EXPECT_EQ(QString("00000.00000.00000.00000.00000.00000.00000.00000.00000.00000"),
              Version("0").toComparableStr());
    EXPECT_EQ(QString("00000.00000.00000.00000.00000.00000.00000.00000.00000.00000"),
              Version("0").toComparableStr());
    EXPECT_EQ(QString("00000.00000.00003.00000.00600.00000.00000.00000.00000.00000"),
              Version("0.0.3.0.600.0").toComparableStr());
}

TEST(VersionTest, testSetVersion)
{
    Version v;

    // valid
    EXPECT_TRUE(v.setVersion("0.1.02.3"));
    EXPECT_EQ(QString("0.1.2.3"), v.toStr());
    EXPECT_TRUE(v.setVersion("0.0.100.0.0"));
    EXPECT_EQ(QString("0.0.100"), v.toStr());

    // invalid
    EXPECT_FALSE(v.setVersion("."));
    EXPECT_EQ(QString(), v.toStr());
    EXPECT_FALSE(v.setVersion("1.2.3.4.5.6.7.8.9.10.11"));
    EXPECT_EQ(QString(), v.toStr());
}

TEST(VersionTest, testOperatorAssign)
{
    Version v1("1.2.3");
    Version v2;
    v2 = v1;
    EXPECT_EQ(v1.isValid(),         v2.isValid());
    EXPECT_EQ(v1.getNumbers(),      v2.getNumbers());
    EXPECT_EQ(v1.toStr(),           v2.toStr());
    EXPECT_EQ(v1.toPrettyStr(0),    v2.toPrettyStr(0));
    EXPECT_EQ(v1.toComparableStr(), v2.toComparableStr());
}

TEST(VersionTest, testOperatorGreater)
{
    EXPECT_TRUE(Version("0.1") > Version("0.0.9"));
    EXPECT_TRUE(Version("5.4") > Version("0.500.0"));
    EXPECT_TRUE(Version("10.0.0.1") > Version("10"));

    EXPECT_FALSE(Version("") > Version(""));
    EXPECT_FALSE(Version("1") > Version(""));
    EXPECT_FALSE(Version("") > Version("1"));
    EXPECT_FALSE(Version("10") > Version("10.0.1"));
    EXPECT_FALSE(Version("0.0.1") > Version("0.1.0"));
}

TEST(VersionTest, testOperatorLess)
{
    EXPECT_TRUE(Version("0.0.9") < Version("0.1"));
    EXPECT_TRUE(Version("0.500.0") < Version("5.4"));
    EXPECT_TRUE(Version("10") < Version("10.0.0.1"));

    EXPECT_FALSE(Version("") < Version(""));
    EXPECT_FALSE(Version("") < Version("1"));
    EXPECT_FALSE(Version("1") < Version(""));
    EXPECT_FALSE(Version("10.0.1") < Version("10"));
    EXPECT_FALSE(Version("0.1.0") < Version("0.0.1"));
}

TEST(VersionTest, testOperatorGreaterEqual)
{
    EXPECT_TRUE(Version("0.1") >= Version("0.0.9"));
    EXPECT_TRUE(Version("5.4") >= Version("0.500.0"));
    EXPECT_TRUE(Version("10.0.0.1") >= Version("10"));
    EXPECT_TRUE(Version("10.0.0.1") >= Version("10.0.0.1"));
    EXPECT_TRUE(Version("5.0.0.5") >= Version("5.0.0.5.0"));

    EXPECT_FALSE(Version("") >= Version(""));
    EXPECT_FALSE(Version("1") >= Version(""));
    EXPECT_FALSE(Version("") >= Version("1"));
    EXPECT_FALSE(Version("10") >= Version("10.0.1"));
    EXPECT_FALSE(Version("0.0.1") >= Version("0.1.0"));
}

TEST(VersionTest, testOperatorLessEqual)
{
    EXPECT_TRUE(Version("0.0.9") <= Version("0.1"));
    EXPECT_TRUE(Version("0.500.0") <= Version("5.4"));
    EXPECT_TRUE(Version("10") <= Version("10.0.0.1"));
    EXPECT_TRUE(Version("10.0.0.1") <= Version("10.0.0.1"));
    EXPECT_TRUE(Version("5.0.0.5") <= Version("5.0.0.5.0"));

    EXPECT_FALSE(Version("") <= Version(""));
    EXPECT_FALSE(Version("") <= Version("1"));
    EXPECT_FALSE(Version("1") <= Version(""));
    EXPECT_FALSE(Version("10.0.1") <= Version("10"));
    EXPECT_FALSE(Version("0.1.0") <= Version("0.0.1"));
}

TEST(VersionTest, testOperatorEqual)
{
    EXPECT_TRUE(Version("10.0.0.1") == Version("10.0.0.1"));
    EXPECT_TRUE(Version("5.0.0.5") == Version("5.0.0.5.0"));

    EXPECT_FALSE(Version("") == Version(""));
    EXPECT_FALSE(Version("") == Version("1"));
    EXPECT_FALSE(Version("1") == Version(""));
    EXPECT_FALSE(Version("10.0.1") == Version("10"));
    EXPECT_FALSE(Version("0.1.0") == Version("0.0.1"));
}

TEST(VersionTest, testOperatorNotEqual)
{
    EXPECT_TRUE(Version("") != Version(""));
    EXPECT_TRUE(Version("") != Version("1"));
    EXPECT_TRUE(Version("1") != Version(""));
    EXPECT_TRUE(Version("10.0.0.1") != Version("10.0.1"));
    EXPECT_TRUE(Version("5.0.5") != Version("0.5.0.5"));

    EXPECT_FALSE(Version("10.0.1") != Version("10.0.1"));
    EXPECT_FALSE(Version("0.1.0") != Version("0.001.0.0.0"));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
