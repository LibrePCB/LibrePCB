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
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/utils/slinthelpers.h>
#include <librepcb/editor/workspace/categorytreemodel.h>

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

class CategoryTreeModelTest : public ::testing::Test {
protected:
  struct Item {
    QString text;
    QVector<Item> childs;
  };
  struct SharedItem {
    QString text;
    QVector<std::shared_ptr<SharedItem>> childs;
  };

  FilePath mWsDir;
  std::unique_ptr<WorkspaceLibraryDb> mWsDb;
  std::unique_ptr<SQLiteDatabase> mDb;
  std::unique_ptr<WorkspaceLibraryDbWriter> mWriter;
  std::unique_ptr<WorkspaceSettings> mSettings;

  CategoryTreeModelTest() : mWsDir(FilePath::getRandomTempPath()) {
    FileUtils::makePath(mWsDir);
    mWsDb.reset(new WorkspaceLibraryDb(mWsDir));
    mDb.reset(new SQLiteDatabase(mWsDb->getFilePath()));
    mWriter.reset(new WorkspaceLibraryDbWriter(mWsDir, *mDb));
    mSettings.reset(new WorkspaceSettings());
  }

  virtual ~CategoryTreeModelTest() { QDir(mWsDir.toStr()).removeRecursively(); }

  std::string str(const std::optional<ui::TreeViewItemData>& data) {
    if (data) {
      return QString("{level=%1, text='%2', hint='%3', user_data='%4'}")
          .arg(data->level)
          .arg(s2q(data->text))
          .arg(s2q(data->hint))
          .arg(s2q(data->user_data))
          .toStdString();
    } else {
      return std::string("{nullopt}");
    }
  }

  std::string str(int level, const QString& text, const QString& hint,
                  const QString& userData) {
    ui::TreeViewItemData item = {};
    item.level = level;
    item.text = q2s(text);
    item.hint = q2s(hint);
    item.user_data = q2s(userData);
    return str(item);
  }

  std::string str(const QVector<Item>& items) {
    std::string s = "[";
    foreach (const auto& child, items) {
      s += child.text.toStdString() + ": " + str(child.childs) + ", ";
    }
    return s + "]";
  }

  QVector<Item> getItems(const CategoryTreeModel& model) {
    QVector<std::shared_ptr<SharedItem>> items;
    QHash<int, std::shared_ptr<SharedItem>> levelItem;
    for (std::size_t i = 0; i < model.row_count(); ++i) {
      if (auto item = model.row_data(i)) {
        auto list = &items;
        if (auto parent = levelItem.value(item->level - 1)) {
          list = &parent->childs;
        }
        list->append(
            std::make_shared<SharedItem>(SharedItem{s2q(item->text), {}}));
        levelItem[item->level] = list->last();
      }
    }
    return toValue(items);
  }

  QVector<Item> toValue(const QVector<std::shared_ptr<SharedItem>>& shared) {
    QVector<Item> result;
    for (const auto& item : shared) {
      result.append(Item{item->text, toValue(item->childs)});
    }
    return result;
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

TEST_F(CategoryTreeModelTest, testDatabaseError) {
  mDb->exec("DROP TABLE component_categories");
  mDb->exec("DROP TABLE package_categories");

  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {
      {"Root Category", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testEmptyDb) {
  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {
      {"Root Category", {}},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testData) {
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

  // Note: Currently we don't show the description as hint.
  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  EXPECT_EQ(str(0, "Root Category", "", "null"), str(model.row_data(0)));
  EXPECT_EQ(str(1, "cat 1", "", uuid(1).toStr()), str(model.row_data(1)));
  EXPECT_EQ(str(2, "cat 2", "", uuid(2).toStr()), str(model.row_data(2)));
}

TEST_F(CategoryTreeModelTest, testComponentCategories) {
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

  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {
      {"Root Category",
       {
           {"cat 1",
            {
                {"cat 2",
                 {
                     {"cat 3", {}},
                     {"cat 4", {}},
                 }},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testPackageCategories) {
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

  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::PkgCat);
  QVector<Item> expected = {
      {"Root Category",
       {
           {"cat 1",
            {
                {"cat 2",
                 {
                     {"cat 3", {}},
                     {"cat 4", {}},
                 }},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testSort) {
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

  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {
      {"Root Category",
       {
           {"cat 9", {}},
           {"cat 10", {}},
           {"cat foo", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testRecursionDirect) {
  // - cat 1 with itself as parent
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);

  // Note: Currently such a category is just not listed in the tree.
  // In future, we may consider to list it anyway in some way.
  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {{"Root Category", {}}};
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testRecursionIndirect) {
  // - cat 1 with cat 2 as parent
  //  - cat 2
  int cat = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, uuid(2));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 1"),
                                             std::nullopt, std::nullopt);
  cat = mWriter->addCategory<ComponentCategory>(0, toAbs("cat2"), uuid(2),
                                                version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(cat, "", ElementName("cat 2"),
                                             std::nullopt, std::nullopt);

  // Note: Currently such a category is just not listed in the tree.
  // In future, we may consider to list it anyway in some way.
  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {{"Root Category", {}}};
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testLiveUpdateAllNew) {
  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {{"Root Category", {}}};
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
  qApp->processEvents();
  qApp->processEvents();
  expected = {
      {"Root Category",
       {
           {"cat 1",
            {
                {"cat 2",
                 {
                     {"cat 3", {}},
                     {"cat 4", {}},
                 }},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testLiveUpdateAllRemoved) {
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

  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {
      {"Root Category",
       {
           {"cat 1",
            {
                {"cat 2",
                 {
                     {"cat 3", {}},
                     {"cat 4", {}},
                 }},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));

  mWriter->removeAllElements<ComponentCategory>();
  emit mWsDb->scanSucceeded(0);  // Triggers a tree model update.
  qApp->processEvents();
  qApp->processEvents();
  qApp->processEvents();
  expected = {{"Root Category", {}}};
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testLiveUpdateVariousModifications) {
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

  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {
      {"Root Category",
       {
           {"cat 1",
            {
                {"cat 2",
                 {
                     {"cat 3", {}},
                     {"cat 4", {}},
                 }},
            }},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));

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
  qApp->processEvents();
  qApp->processEvents();
  expected = {
      {"Root Category",
       {
           {"cat 1 renamed",
            {
                {"cat 5",
                 {
                     {"cat 6", {}},
                 }},
            }},
           {"cat 7", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

TEST_F(CategoryTreeModelTest, testSetLocaleOrder) {
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

  CategoryTreeModel model(*mWsDb, *mSettings,
                          CategoryTreeModel::Filter::CmpCat);
  QVector<Item> expected = {
      {"Root Category",
       {
           {"cat 1", {}},
           {"cat 2", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));

  mSettings->libraryLocaleOrder.set({"de_CH"});
  qApp->processEvents();
  qApp->processEvents();
  qApp->processEvents();

  expected = {
      {"Root Category",
       {
           {"cat 0", {}},
           {"cat 1", {}},
       }},
  };
  EXPECT_EQ(str(expected), str(getItems(model)));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
