/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <eda4ucommon/fileio/filepath.h>

namespace tests {

/*****************************************************************************************
 *  Test Data Type
 ****************************************************************************************/

typedef struct {
    bool valid;
    QString inputFilePath;
    QString inputBasePath;  // used to test toRelative() and fromRelative()
    QString toStr;
    QString toWindowsStyle; // used to test toNative() on Windows
    QString toRelative;
} Data_t;

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class FilePathTest : public ::testing::TestWithParam<Data_t>
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_P(FilePathTest, testDefaultConstructor)
{
    FilePath p;
    EXPECT_FALSE(p.isValid());
    EXPECT_EQ(QString(""), p.toStr());
}

TEST_P(FilePathTest, testConstructor)
{
    const Data_t& data = GetParam();

    FilePath p(data.inputFilePath);
    EXPECT_EQ(data.valid, p.isValid());
    EXPECT_EQ(data.toStr, p.toStr());
}

TEST_P(FilePathTest, testCopyConstructor)
{
    const Data_t& data = GetParam();

    FilePath p1(data.inputFilePath);
    FilePath p2(p1);
    EXPECT_EQ(p1.isValid(), p2.isValid());
    EXPECT_EQ(p1.toStr(), p2.toStr());
}

TEST_P(FilePathTest, testSetPath)
{
    const Data_t& data = GetParam();

    FilePath p;
    EXPECT_EQ(data.valid, p.setPath(data.inputFilePath));
    EXPECT_EQ(data.valid, p.isValid());
    EXPECT_EQ(data.toStr, p.toStr());
}

TEST_P(FilePathTest, testToStr)
{
    const Data_t& data = GetParam();

    FilePath p(data.inputFilePath);
    EXPECT_EQ(data.toStr, p.toStr());
}

TEST_P(FilePathTest, testToNative)
{
    const Data_t& data = GetParam();

    FilePath p(data.inputFilePath);
#ifdef Q_OS_WIN
    EXPECT_EQ(data.toWindowsStyle, p.toNative());
#else
    EXPECT_EQ(data.toStr, p.toNative());
#endif
}

TEST_P(FilePathTest, testToRelative)
{
    const Data_t& data = GetParam();

    if (data.valid)
    {
        FilePath base(data.inputBasePath);
        FilePath p(data.inputFilePath);
        EXPECT_EQ(data.toRelative, p.toRelative(base));
    }
}

TEST_P(FilePathTest, testFromRelative)
{
    const Data_t& data = GetParam();

    if (data.valid)
    {
        FilePath base(data.inputBasePath);
        FilePath p = FilePath::fromRelative(base, data.toRelative);
        EXPECT_EQ(data.toStr, p.toStr());
    }
}

TEST_P(FilePathTest, testOperatorAssign)
{
    const Data_t& data = GetParam();

    FilePath p1(data.inputFilePath);
    FilePath p2("/valid/path");
    p2 = p1;
    EXPECT_EQ(p1.isValid(), p2.isValid());
    EXPECT_EQ(p1.toStr(), p2.toStr());
}

/*****************************************************************************************
 *  Test Data
 ****************************************************************************************/

INSTANTIATE_TEST_CASE_P(FilePathTest, FilePathTest, ::testing::Values(

    // valid paths
    //     {valid, "inputFilePath"         , "inputBasePath"  , "toStr"           , "toWindowsStyle"      , "toRelative"      }
#ifdef Q_OS_WIN
    Data_t({true , "C:\\foo\\bar"          , "C:/foo"         , "C:/foo/bar"      , "C:\\foo\\bar"        , "bar"             }), // Win path to a dir
    Data_t({true , "C:\\foo\\bar\\"        , "C:/bar"         , "C:/foo/bar"      , "C:\\foo\\bar"        , "../foo/bar"      }), // Win path to a dir + backslash
    Data_t({true , "C:\\foo\\bar.txt"      , "C:/bar"         , "C:/foo/bar.txt"  , "C:\\foo\\bar.txt"    , "../foo/bar.txt"  }), // Win path to a file
    Data_t({true , "C:\\foo\\bar"          , "C:/foo\\bar"    , "C:/foo/bar"      , "C:\\foo\\bar"        , ""                }), // Win path with path==base
    Data_t({true , "C:\\\\foo\\..\\bar\\"  , "C:\\"           , "C:/bar"          , "C:\\bar"             , "bar"             }), // Win path with .. and double backslashes
    Data_t({true , "C:\\"                  , "C:\\foo"        , "C:"              , "C:"                  , ".."              }), // Win drive root path
#endif
    Data_t({true , "/foo/bar"              , "/foo"           , "/foo/bar"        , "\\foo\\bar"          , "bar"             }), // UNIX path to a dir
    Data_t({true , "/foo/bar/"             , "/bar"           , "/foo/bar"        , "\\foo\\bar"          , "../foo/bar"      }), // UNIX path to a dir + slash
    Data_t({true , "/foo/bar.txt"          , "/bar"           , "/foo/bar.txt"    , "\\foo\\bar.txt"      , "../foo/bar.txt"  }), // UNIX path to a file
    Data_t({true , "/foo/bar"              , "/foo/bar"       , "/foo/bar"        , "\\foo\\bar"          , ""                }), // UNIX path with path==base
    Data_t({true , "//foo/..//bar//"       , "/"              , "/bar"            , "\\bar"               , "bar"             }), // UNIX path with .. and double slashes
    Data_t({true , "/"                     , "/foo"           , "/"               , "\\"                  , ".."              }), // UNIX root path

    // invalid paths
    //     {valid, "inputFilePath"         , "inputBasePath"  , "toStr"           , "toWindowsStyle"      , "toRelative"      }
#ifdef Q_OS_WIN
    Data_t({false, "foo\\bar"              , ""               , ""                , ""                    , ""                }), // rel. Win path to a dir
    Data_t({false, "foo\\bar.txt"          , ""               , ""                , ""                    , ""                }), // rel. Win path to a file
#endif
    Data_t({false, "foo/bar"               , ""               , ""                , ""                    , ""                }), // rel. UNIX path to a dir
    Data_t({false, "foo/bar.txt"           , ""               , ""                , ""                    , ""                }), // rel. UNIX path to a file
    Data_t({false, ""                      , ""               , ""                , ""                    , ""                })  // empty path
));

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
