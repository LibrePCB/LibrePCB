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
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/sqlitedatabase.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacelibrarydbwriter.h>
#include <librepcb/editor/library/cat/categorytreebuilder.h>

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

class CategoryTreeBuilderTest : public ::testing::Test {
protected:
  FilePath mWsDir;
  std::unique_ptr<WorkspaceLibraryDb> mWsDb;
  std::unique_ptr<SQLiteDatabase> mDb;
  std::unique_ptr<WorkspaceLibraryDbWriter> mWriter;

  CategoryTreeBuilderTest() : mWsDir(FilePath::getRandomTempPath()) {
    FileUtils::makePath(mWsDir);
    mWsDb.reset(new WorkspaceLibraryDb(mWsDir));
    mDb.reset(new SQLiteDatabase(mWsDb->getFilePath()));
    mWriter.reset(new WorkspaceLibraryDbWriter(mWsDir, *mDb));
  }

  virtual ~CategoryTreeBuilderTest() {
    QDir(mWsDir.toStr()).removeRecursively();
  }

  std::string str(const QStringList& list) {
    return list.join(", ").toStdString();
  }

  FilePath toAbs(const QString& fp) { return mWsDir.getPathTo(fp); }

  Uuid uuid(int index = -1) {
    static QHash<int, Uuid> cache;
    if (index >= 0) {
      auto it = cache.find(index);
      if (it == cache.end()) {
        it = cache.insert(index, uuid());
      }
      return *it;
    } else {
      return Uuid::createRandom();
    }
  }

  Version version(const QString& version) {
    return Version::fromString(version);
  }

  template <typename ElementType>
  void test(const QStringList& localeOrder, bool nulloptIsRootCategory,
            const tl::optional<Uuid>& category, bool expSuccess,
            const QStringList& expOutput) {
    CategoryTreeBuilder<ElementType> builder(*mWsDb, localeOrder,
                                             nulloptIsRootCategory);
    bool retSuccess = !expSuccess;
    QStringList output = builder.buildTree(category, &retSuccess);
    EXPECT_EQ(str(expOutput), str(output));
    EXPECT_EQ(expSuccess, retSuccess);
  }
};

/*******************************************************************************
 *  Tests
 ******************************************************************************/

TEST_F(CategoryTreeBuilderTest, testDatabaseError) {
  mDb->exec("DROP TABLE component_categories");
  mDb->exec("DROP TABLE package_categories");

  EXPECT_THROW(test<ComponentCategory>({}, false, uuid(), false, {}),
               RuntimeError);
  EXPECT_THROW(test<PackageCategory>({}, false, uuid(), false, {}),
               RuntimeError);
}

TEST_F(CategoryTreeBuilderTest, testEmptyDbNull) {
  test<ComponentCategory>({}, false, tl::nullopt, true, {});
  test<PackageCategory>({}, false, tl::nullopt, true, {});
}

TEST_F(CategoryTreeBuilderTest, testEmptyRootDbNull) {
  test<ComponentCategory>({}, true, tl::nullopt, true, {"Root category"});
  test<PackageCategory>({}, true, tl::nullopt, true, {"Root category"});
}

TEST_F(CategoryTreeBuilderTest, testInexistent) {
  Uuid uuid = Uuid::fromString("a39c1053-cbd3-478b-8455-57dff69c6375");
  test<ComponentCategory>({}, false, uuid, false,
                          {"ERROR: a39c1053 not found"});
  test<PackageCategory>({}, false, uuid, false, {"ERROR: a39c1053 not found"});
  test<ComponentCategory>({}, true, uuid, false, {"ERROR: a39c1053 not found"});
  test<PackageCategory>({}, true, uuid, false, {"ERROR: a39c1053 not found"});
}

TEST_F(CategoryTreeBuilderTest, testInexistentParent) {
  Uuid parentUuid = Uuid::fromString("a39c1053-cbd3-478b-8455-57dff69c6375");
  int cmpCat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cmpcat"), uuid(1), version("0.1"), false, parentUuid);
  mWriter->addTranslation<ComponentCategory>(cmpCat, "", ElementName("cmp cat"),
                                             tl::nullopt, tl::nullopt);
  int pkgCat = mWriter->addCategory<PackageCategory>(
      0, toAbs("pkgcat"), uuid(2), version("0.1"), false, parentUuid);
  mWriter->addTranslation<PackageCategory>(pkgCat, "", ElementName("pkg cat"),
                                           tl::nullopt, tl::nullopt);

  test<ComponentCategory>({}, false, uuid(1), false,
                          {"ERROR: a39c1053 not found", "cmp cat"});
  test<PackageCategory>({}, false, uuid(2), false,
                        {"ERROR: a39c1053 not found", "pkg cat"});
  test<ComponentCategory>({}, true, uuid(1), false,
                          {"ERROR: a39c1053 not found", "cmp cat"});
  test<PackageCategory>({}, true, uuid(2), false,
                        {"ERROR: a39c1053 not found", "pkg cat"});
}

// Note: Tests above have shown that the class works for both, ComponentCategory
// and PackageCategory. Thus the detailed tests below now only test with
// ComponentCategory (for simplicity).

TEST_F(CategoryTreeBuilderTest, testNullptr) {
  CategoryTreeBuilder<ComponentCategory> builder(*mWsDb, {}, false);
  QStringList output = builder.buildTree(tl::nullopt);
  EXPECT_EQ(str({}), str(output));
}

TEST_F(CategoryTreeBuilderTest, testLocaleOrder) {
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             tl::nullopt, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(
      cat, "de_DE", ElementName("cat 1 de"), tl::nullopt, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(
      cat, "it_IT", ElementName("cat 1 it"), tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(3));
  mWriter->addTranslation<ComponentCategory>(
      cat, "it_IT", ElementName("cat 2 it"), tl::nullopt, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             tl::nullopt, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(
      cat, "de_CH", ElementName("cat 2 ch"), tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat3"), uuid(3), version("0.1"), false, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             tl::nullopt, tl::nullopt);

  test<ComponentCategory>({"fr_FR", "de_CH", "de_DE"}, false, uuid(1), true,
                          {"cat 3", "cat 2 ch", "cat 1 de"});
  test<ComponentCategory>({"fr_FR", "de_CH", "de_DE"}, true, uuid(1), true,
                          {"Root category", "cat 3", "cat 2 ch", "cat 1 de"});
}

TEST_F(CategoryTreeBuilderTest, testMultipleParents) {
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(3));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(4));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat4"), uuid(4), version("0.1"), false, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 4"),
                                             tl::nullopt, tl::nullopt);

  test<ComponentCategory>({}, false, uuid(1), true,
                          {"cat 4", "cat 3", "cat 2", "cat 1"});
  test<ComponentCategory>(
      {}, true, uuid(1), true,
      {"Root category", "cat 4", "cat 3", "cat 2", "cat 1"});
}

TEST_F(CategoryTreeBuilderTest, testEndlessRecursionDirect) {
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             tl::nullopt, tl::nullopt);

  test<ComponentCategory>({}, false, uuid(1), false,
                          {"ERROR: Endless recursion", "cat 2", "cat 1"});
  test<ComponentCategory>({}, true, uuid(1), false,
                          {"ERROR: Endless recursion", "cat 2", "cat 1"});
}

TEST_F(CategoryTreeBuilderTest, testEndlessRecursionMultipleParents) {
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(3));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(4));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             tl::nullopt, tl::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat4"), uuid(4),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 4"),
                                             tl::nullopt, tl::nullopt);

  test<ComponentCategory>(
      {}, false, uuid(1), false,
      {"ERROR: Endless recursion", "cat 4", "cat 3", "cat 2", "cat 1"});
  test<ComponentCategory>(
      {}, true, uuid(1), false,
      {"ERROR: Endless recursion", "cat 4", "cat 3", "cat 2", "cat 1"});
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
