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
#include <librepcb/editor/workspace/categorytreemodellegacy.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class CategoryTreeModelLegacyTest : public ::testing::Test {
protected:
  struct Item {
    QString text;
    QVector<Item> childs;
  };

  FilePath mWsDir;
  std::unique_ptr<WorkspaceLibraryDb> mWsDb;
  std::unique_ptr<SQLiteDatabase> mDb;
  std::unique_ptr<WorkspaceLibraryDbWriter> mWriter;

  CategoryTreeModelLegacyTest() : mWsDir(FilePath::getRandomTempPath()) {
    FileUtils::makePath(mWsDir);
    mWsDb.reset(new WorkspaceLibraryDb(mWsDir));
    mDb.reset(new SQLiteDatabase(mWsDb->getFilePath()));
    mWriter.reset(new WorkspaceLibraryDbWriter(mWsDir, *mDb));
  }

  virtual ~CategoryTreeModelLegacyTest() {
    QDir(mWsDir.toStr()).removeRecursively();
  }

  std::string str(const QVariant& data) {
    return data.toString().toStdString();
  }

  std::string str(const Uuid& uuid) { return uuid.toStr().toStdString(); }

  std::string str(const QVector<Item>& items) {
    std::string s = "[";
    foreach (const Item& child, items) {
      s += child.text.toStdString() + ": " + str(child.childs) + ", ";
    }
    return s + "]";
  }

  QVector<Item> getItems(const CategoryTreeModelLegacy& model,
                         const QModelIndex& index = QModelIndex()) {
    QVector<Item> items;
    for (int i = 0; i < model.rowCount(index); ++i) {
      QModelIndex child = model.index(i, 0, index);
      items.append(Item{child.data().toString(), getItems(model, child)});
    }
    return items;
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
};

/*******************************************************************************
 *  Tests
 ******************************************************************************/

TEST_F(CategoryTreeModelLegacyTest, testDatabaseError) {
  mDb->exec("DROP TABLE component_categories");
  mDb->exec("DROP TABLE package_categories");

  CategoryTreeModelLegacy model(
      *mWsDb, {},
      CategoryTreeModelLegacy::Filter::CmpCat |
          CategoryTreeModelLegacy::Filter::CmpCatWithComponents);
  EXPECT_EQ(str(QVector<Item>{}), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testEmptyDb) {
  CategoryTreeModelLegacy model(
      *mWsDb, {},
      CategoryTreeModelLegacy::Filter::CmpCat |
          CategoryTreeModelLegacy::Filter::CmpCatWithComponents);
  EXPECT_EQ(str(QVector<Item>{}), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testData) {
  // - cat 1
  //   - cat 2
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             "desc 1", std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::CmpCat);
  QModelIndex i1 = model.index(0, 0);
  EXPECT_EQ("cat 1", str(i1.data(Qt::DisplayRole)));
  EXPECT_EQ("desc 1", str(i1.data(Qt::ToolTipRole)));
  EXPECT_EQ(str(uuid(1)), str(i1.data(Qt::UserRole)));
  QModelIndex i2 = model.index(0, 0, i1);
  EXPECT_EQ("cat 2", str(i2.data(Qt::DisplayRole)));
  EXPECT_EQ("", str(i2.data(Qt::ToolTipRole)));
  EXPECT_EQ(str(uuid(2)), str(i2.data(Qt::UserRole)));
}

TEST_F(CategoryTreeModelLegacyTest, testComponentCategories) {
  // - cat 1
  //   - cat 2
  //     - cat 3
  //     - cat 4
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat4"), uuid(4),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 4"),
                                             std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::CmpCat);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 2",
            {
                {"cat 3", {}},
                {"cat 4", {}},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testPackageCategories) {
  // - cat 1
  //   - cat 2
  //     - cat 3
  //     - cat 4
  int cat = mWriter->addCategory<PackageCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 1"),
                                           std::nullopt, std::nullopt);
  cat = mWriter->addCategory<PackageCategory>(0, toAbs("cat2"), uuid(2),
                                              version("0.1"), false, uuid(1));
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 2"),
                                           std::nullopt, std::nullopt);
  cat = mWriter->addCategory<PackageCategory>(0, toAbs("cat3"), uuid(3),
                                              version("0.1"), false, uuid(2));
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 3"),
                                           std::nullopt, std::nullopt);
  cat = mWriter->addCategory<PackageCategory>(0, toAbs("cat4"), uuid(4),
                                              version("0.1"), false, uuid(2));
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 4"),
                                           std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::PkgCat);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 2",
            {
                {"cat 3", {}},
                {"cat 4", {}},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testSort) {
  // - cat 9
  // - cat 10
  // - cat foo
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat foo"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat2"), uuid(2), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 10"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat3"), uuid(3), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 9"),
                                             std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::CmpCat);
  QVector<Item> expected = {
      {"cat 9", {}},
      {"cat 10", {}},
      {"cat foo", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testCmpCatWithEmpty) {
  // - cat 1 (0 elements)
  //   - cat 2 (0 elements)
  //   - cat 3 (0 elements)
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(
      *mWsDb, {},
      CategoryTreeModelLegacy::Filter::CmpCatWithSymbols |
          CategoryTreeModelLegacy::Filter::CmpCatWithComponents |
          CategoryTreeModelLegacy::Filter::CmpCatWithDevices);
  EXPECT_EQ(str(QVector<Item>{}), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testCmpCatWithSymbols) {
  // - cat 1 (0 elements)
  //   - cat 2 (0 elements)
  //   - cat 3 (1 elements)
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);
  int sym = mWriter->addElement<Symbol>(0, toAbs("sym"), uuid(), version("0.1"),
                                        false, QString());
  mWriter->addToCategory<Symbol>(sym, uuid(3));

  CategoryTreeModelLegacy model(
      *mWsDb, {}, CategoryTreeModelLegacy::Filter::CmpCatWithSymbols);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 3", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testCmpCatWithComponents) {
  // - cat 1 (0 elements)
  //   - cat 2 (0 elements)
  //   - cat 3 (1 elements)
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);
  int cmp = mWriter->addElement<Component>(0, toAbs("cmp"), uuid(),
                                           version("0.1"), false, QString());
  mWriter->addToCategory<Component>(cmp, uuid(3));

  CategoryTreeModelLegacy model(
      *mWsDb, {}, CategoryTreeModelLegacy::Filter::CmpCatWithComponents);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 3", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testCmpCatWithDevices) {
  // - cat 1 (0 elements)
  //   - cat 2 (0 elements)
  //   - cat 3 (1 elements)
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);
  int dev = mWriter->addDevice(0, toAbs("dev"), uuid(), version("0.1"), false,
                               QString(), uuid(), uuid());
  mWriter->addToCategory<Device>(dev, uuid(3));

  CategoryTreeModelLegacy model(
      *mWsDb, {}, CategoryTreeModelLegacy::Filter::CmpCatWithDevices);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 3", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testPkgCatWithEmpty) {
  // - cat 1 (0 elements)
  //   - cat 2 (0 elements)
  //   - cat 3 (0 elements)
  int cat = mWriter->addCategory<PackageCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 1"),
                                           std::nullopt, std::nullopt);
  cat = mWriter->addCategory<PackageCategory>(0, toAbs("cat2"), uuid(2),
                                              version("0.1"), false, uuid(1));
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 2"),
                                           std::nullopt, std::nullopt);
  cat = mWriter->addCategory<PackageCategory>(0, toAbs("cat3"), uuid(3),
                                              version("0.1"), false, uuid(1));
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 3"),
                                           std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(
      *mWsDb, {}, CategoryTreeModelLegacy::Filter::PkgCatWithPackages);
  EXPECT_EQ(str(QVector<Item>{}), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testPkgCatWithPackages) {
  // - cat 1 (0 elements)
  //   - cat 2 (0 elements)
  //   - cat 3 (1 elements)
  int cat = mWriter->addCategory<PackageCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 1"),
                                           std::nullopt, std::nullopt);
  cat = mWriter->addCategory<PackageCategory>(0, toAbs("cat2"), uuid(2),
                                              version("0.1"), false, uuid(1));
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 2"),
                                           std::nullopt, std::nullopt);
  cat = mWriter->addCategory<PackageCategory>(0, toAbs("cat3"), uuid(3),
                                              version("0.1"), false, uuid(1));
  mWriter->addTranslation<PackageCategory>(cat, "", ElementName("cat 3"),
                                           std::nullopt, std::nullopt);
  int pkg = mWriter->addElement<Package>(0, toAbs("pkg"), uuid(),
                                         version("0.1"), false, QString());
  mWriter->addToCategory<Package>(pkg, uuid(3));

  CategoryTreeModelLegacy model(
      *mWsDb, {}, CategoryTreeModelLegacy::Filter::PkgCatWithPackages);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 3", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testWithoutCategory) {
  mWriter->addDevice(0, toAbs("dev"), uuid(), version("0.1"), false, QString(),
                     uuid(), uuid());

  CategoryTreeModelLegacy model(
      *mWsDb, {}, CategoryTreeModelLegacy::Filter::CmpCatWithDevices);
  QVector<Item> expected = {
      {"(Without Category)", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testInexistentCategory) {
  int dev = mWriter->addDevice(0, toAbs("dev"), uuid(), version("0.1"), false,
                               QString(), uuid(), uuid());
  mWriter->addToCategory<Device>(dev, uuid(1));  // Inexistent category.

  CategoryTreeModelLegacy model(
      *mWsDb, {}, CategoryTreeModelLegacy::Filter::CmpCatWithDevices);
  QVector<Item> expected = {
      {"(Without Category)", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testLiveUpdateAllNew) {
  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::CmpCat);
  QVector<Item> expected = {};
  EXPECT_EQ(str(expected), str(getItems(model)));

  // - cat 1
  //   - cat 2
  //     - cat 3
  //     - cat 4
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat4"), uuid(4),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 4"),
                                             std::nullopt, std::nullopt);

  emit mWsDb->scanSucceeded(0);  // Triggers a tree model update.
  qApp->processEvents();
  expected = {
      {"cat 1",
       {
           {"cat 2",
            {
                {"cat 3", {}},
                {"cat 4", {}},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testLiveUpdateAllRemoved) {
  // - cat 1
  //   - cat 2
  //     - cat 3
  //     - cat 4
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat4"), uuid(4),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 4"),
                                             std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::CmpCat);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 2",
            {
                {"cat 3", {}},
                {"cat 4", {}},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));

  mWriter->removeAllElements<ComponentCategory>();
  emit mWsDb->scanSucceeded(0);  // Triggers a tree model update.
  qApp->processEvents();
  expected = {};
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelLegacyTest, testLiveUpdateVariousModifications) {
  // - cat 1
  //   - cat 2
  //     - cat 3
  //     - cat 4
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat3"), uuid(3),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 3"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat4"), uuid(4),
                                                version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 4"),
                                             std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::CmpCat);
  QVector<Item> expected = {
      {"cat 1",
       {
           {"cat 2",
            {
                {"cat 3", {}},
                {"cat 4", {}},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));

  // Show the model in a QTreeView and select an item which gets removed,
  // just to ensure the model update also works while a view is connected.
  QTreeView view;
  view.setModel(&model);
  view.show();
  view.setCurrentIndex(model.index(0, 0, model.index(0, 0)));  // cat 2
  EXPECT_TRUE(view.currentIndex().isValid());
  qApp->processEvents();

  // - cat 1 renamed
  //   - cat 5
  //     - cat 6
  // - cat 7
  mWriter->removeAllElements<ComponentCategory>();
  cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(
      cat, "", ElementName("cat 1 renamed"), std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat5"), uuid(5),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 5"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat6"), uuid(6),
                                                version("0.1"), false, uuid(5));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 6"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat7"), uuid(7), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 7"),
                                             std::nullopt, std::nullopt);

  emit mWsDb->scanSucceeded(0);  // Triggers a tree model update.
  qApp->processEvents();
  expected = {
      {"cat 1 renamed",
       {
           {"cat 5",
            {
                {"cat 6", {}},
            }},
       }},
      {"cat 7", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));

  // Verify that the selection was updated in a reasonable way.
  EXPECT_EQ("cat 5", str(view.currentIndex().data()));
}

TEST_F(CategoryTreeModelLegacyTest, testSetLocaleOrder) {
  // - cat 1
  // - cat 2, cat 0 (de_CH)
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat2"), uuid(2), version("0.1"), false, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);
  mWriter->addTranslation<ComponentCategory>(cat, "de_CH", ElementName("cat 0"),
                                             std::nullopt, std::nullopt);

  CategoryTreeModelLegacy model(*mWsDb, {},
                                CategoryTreeModelLegacy::Filter::CmpCat);
  QVector<Item> expected = {
      {"cat 1", {}},
      {"cat 2", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));

  model.setLocaleOrder({"de_CH"});

  expected = {
      {"cat 0", {}},
      {"cat 1", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
