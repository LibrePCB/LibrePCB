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
#include <librepcbcommon/fileio/filepath.h>

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
    QString inputFilePath;
    QString inputBasePath;  // used to test toRelative() and fromRelative()
    QString toStr;
    QString toWindowsStyle; // used to test toNative() on Windows
    QString toRelative;
} FilePathTestData;

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class FilePathTest : public ::testing::TestWithParam<FilePathTestData>
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
    const FilePathTestData& data = GetParam();

    FilePath p(data.inputFilePath);
    EXPECT_EQ(data.valid, p.isValid());
    EXPECT_EQ(data.toStr, p.toStr());
}

TEST_P(FilePathTest, testCopyConstructor)
{
    const FilePathTestData& data = GetParam();

    FilePath p1(data.inputFilePath);
    FilePath p2(p1);
    EXPECT_EQ(p1.isValid(), p2.isValid());
    EXPECT_EQ(p1.toStr(), p2.toStr());
}

TEST_P(FilePathTest, testSetPath)
{
    const FilePathTestData& data = GetParam();

    FilePath p;
    EXPECT_EQ(data.valid, p.setPath(data.inputFilePath));
    EXPECT_EQ(data.valid, p.isValid());
    EXPECT_EQ(data.toStr, p.toStr());
}

TEST_P(FilePathTest, testToStr)
{
    const FilePathTestData& data = GetParam();

    FilePath p(data.inputFilePath);
    EXPECT_EQ(data.toStr, p.toStr());
}

TEST_P(FilePathTest, testToNative)
{
    const FilePathTestData& data = GetParam();

    FilePath p(data.inputFilePath);
#ifdef Q_OS_WIN
    EXPECT_EQ(data.toWindowsStyle, p.toNative());
#else
    EXPECT_EQ(data.toStr, p.toNative());
#endif
}

TEST_P(FilePathTest, testToRelative)
{
    const FilePathTestData& data = GetParam();

    if (data.valid)
    {
        FilePath base(data.inputBasePath);
        FilePath p(data.inputFilePath);
        EXPECT_EQ(data.toRelative, p.toRelative(base));
    }
}

TEST_P(FilePathTest, testFromRelative)
{
    const FilePathTestData& data = GetParam();

    if (data.valid)
    {
        FilePath base(data.inputBasePath);
        FilePath p = FilePath::fromRelative(base, data.toRelative);
        EXPECT_EQ(data.toStr, p.toStr());
    }
}

TEST_P(FilePathTest, testOperatorAssign)
{
    const FilePathTestData& data = GetParam();

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

    // valid paths   {valid, "inputFilePath"         , "inputBasePath"  , "toStr"           , "toWindowsStyle"      , "toRelative"      }
#ifdef Q_OS_WIN
    FilePathTestData({true , "C:\\foo\\bar"          , "C:/foo"         , "C:/foo/bar"      , "C:\\foo\\bar"        , "bar"             }), // Win path to a dir
    FilePathTestData({true , "C:\\foo\\bar\\"        , "C:/bar"         , "C:/foo/bar"      , "C:\\foo\\bar"        , "../foo/bar"      }), // Win path to a dir + backslash
    FilePathTestData({true , "C:\\foo\\bar.txt"      , "C:/bar"         , "C:/foo/bar.txt"  , "C:\\foo\\bar.txt"    , "../foo/bar.txt"  }), // Win path to a file
    FilePathTestData({true , "C:\\foo\\bar"          , "C:/foo\\bar"    , "C:/foo/bar"      , "C:\\foo\\bar"        , ""                }), // Win path with path==base
    FilePathTestData({true , "C:\\\\foo\\..\\bar\\"  , "C:\\"           , "C:/bar"          , "C:\\bar"             , "bar"             }), // Win path with .. and double backslashes
    FilePathTestData({true , "C:\\"                  , "C:\\foo"        , "C:"              , "C:"                  , ".."              }), // Win drive root path
#endif
    FilePathTestData({true , "/foo/bar"              , "/foo"           , "/foo/bar"        , "\\foo\\bar"          , "bar"             }), // UNIX path to a dir
    FilePathTestData({true , "/foo/bar/"             , "/bar"           , "/foo/bar"        , "\\foo\\bar"          , "../foo/bar"      }), // UNIX path to a dir + slash
    FilePathTestData({true , "/foo/bar.txt"          , "/bar"           , "/foo/bar.txt"    , "\\foo\\bar.txt"      , "../foo/bar.txt"  }), // UNIX path to a file
    FilePathTestData({true , "/foo/bar"              , "/foo/bar"       , "/foo/bar"        , "\\foo\\bar"          , ""                }), // UNIX path with path==base
/// @TODO: this test fails on Windows --> fix this!
//    FilePathTestData({true , "//foo/..//bar//"       , "/"              , "/bar"            , "\\bar"               , "bar"             }), // UNIX path with .. and double slashes
    FilePathTestData({true , "/"                     , "/foo"           , "/"               , "\\"                  , ".."              }), // UNIX root path

    // invalid paths {valid, "inputFilePath"         , "inputBasePath"  , "toStr"           , "toWindowsStyle"      , "toRelative"      }
#ifdef Q_OS_WIN
    FilePathTestData({false, "foo\\bar"              , ""               , ""                , ""                    , ""                }), // rel. Win path to a dir
    FilePathTestData({false, "foo\\bar.txt"          , ""               , ""                , ""                    , ""                }), // rel. Win path to a file
#endif
    FilePathTestData({false, "foo/bar"               , ""               , ""                , ""                    , ""                }), // rel. UNIX path to a dir
    FilePathTestData({false, "foo/bar.txt"           , ""               , ""                , ""                    , ""                }), // rel. UNIX path to a file
    FilePathTestData({false, ""                      , ""               , ""                , ""                    , ""                })  // empty path
));

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
