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

#include "../../testhelpers.h"

#include <gtest/gtest.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/sqlitedatabase.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacelibrarydbwriter.h>
#include <librepcb/editor/project/addcomponentdialog.h>

#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

using ::librepcb::tests::TestHelpers;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AddComponentDialogTest : public ::testing::Test {
protected:
  FilePath mWsDir;
  std::unique_ptr<WorkspaceLibraryDb> mWsDb;
  std::unique_ptr<SQLiteDatabase> mDb;
  std::unique_ptr<WorkspaceLibraryDbWriter> mWriter;
  std::shared_ptr<TransactionalFileSystem> mFs;

  AddComponentDialogTest() : mWsDir(FilePath::getRandomTempPath()) {
    QSettings().clear();
    FileUtils::makePath(mWsDir);
    mWsDb.reset(new WorkspaceLibraryDb(mWsDir));
    mDb.reset(new SQLiteDatabase(mWsDb->getFilePath()));
    mWriter.reset(new WorkspaceLibraryDbWriter(mWsDir, *mDb));
    mFs.reset(new TransactionalFileSystem(mWsDir, true));
  }

  std::string str(const tl::optional<Uuid>& uuid) {
    return uuid ? uuid->toStr().toStdString() : "";
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
 *  Test Methods
 ******************************************************************************/

TEST_F(AddComponentDialogTest, testAddMore) {
  const bool defaultValue = true;
  const bool newValue = false;

  {
    AddComponentDialog dialog(*mWsDb, {}, {});

    // Check the default value.
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dialog, "cbxAddMore");
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, dialog.getAutoOpenAgain());

    // Check if the value can be changed.
    cbx.setChecked(newValue);
    EXPECT_EQ(newValue, dialog.getAutoOpenAgain());
  }

  // Check if the setting is saved and restored automatically.
  {
    AddComponentDialog dialog(*mWsDb, {}, {});
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dialog, "cbxAddMore");
    EXPECT_EQ(newValue, cbx.isChecked());
    EXPECT_EQ(newValue, dialog.getAutoOpenAgain());
  }
}

TEST_F(AddComponentDialogTest, testChooseComponentDevice) {
  // - cat 1
  //   - cat 2
  //     - cmp 1
  //     - cmp 2
  //       - dev 1
  int catId = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(catId, "", ElementName("cat 1"),
                                             tl::nullopt, tl::nullopt);
  catId = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat2"), uuid(2), version("0.1"), false, uuid(1));
  mWriter->addTranslation<ComponentCategory>(catId, "", ElementName("cat 2"),
                                             tl::nullopt, tl::nullopt);
  int cmpId = mWriter->addElement<Component>(0, toAbs("cmp1"), uuid(3),
                                             version("0.1"), false);
  mWriter->addTranslation<Component>(cmpId, "", ElementName("cmp 1"),
                                     tl::nullopt, tl::nullopt);
  mWriter->addToCategory<Component>(cmpId, uuid(2));
  cmpId = mWriter->addElement<Component>(0, toAbs(uuid(4).toStr()), uuid(4),
                                         version("0.1"), false);
  mWriter->addTranslation<Component>(cmpId, "", ElementName("cmp 2"),
                                     tl::nullopt, tl::nullopt);
  mWriter->addToCategory<Component>(cmpId, uuid(2));
  int pkgId = mWriter->addElement<Package>(0, toAbs(uuid(5).toStr()), uuid(5),
                                           version("0.1"), false);
  mWriter->addTranslation<Package>(pkgId, "", ElementName("pkg 1"), tl::nullopt,
                                   tl::nullopt);
  int devId = mWriter->addDevice(0, toAbs(uuid(6).toStr()), uuid(6),
                                 version("0.1"), false, uuid(4), uuid(5));
  mWriter->addTranslation<Device>(devId, "", ElementName("dev 1"), tl::nullopt,
                                  tl::nullopt);

  // Create component
  TransactionalDirectory cmp2Dir(mFs, uuid(4).toStr());
  Component cmp2(uuid(4), version("0.1"), "", ElementName("cmp 2"), "", "");
  auto cmp2SymbVar1 = std::make_shared<ComponentSymbolVariant>(
      uuid(7), "", ElementName("var 1"), "");
  cmp2.getSymbolVariants().append(cmp2SymbVar1);
  auto cmp2SymbVar2 = std::make_shared<ComponentSymbolVariant>(
      uuid(8), "", ElementName("var 2"), "");
  cmp2.getSymbolVariants().append(cmp2SymbVar2);
  cmp2.saveTo(cmp2Dir);

  // Create package
  TransactionalDirectory pkg1Dir(mFs, uuid(5).toStr());
  Package pkg1(uuid(5), version("0.1"), "", ElementName("pkg 1"), "", "");
  pkg1.saveTo(pkg1Dir);

  // Create device
  TransactionalDirectory dev1Dir(mFs, uuid(6).toStr());
  Device dev1(uuid(6), version("0.1"), "", ElementName("dev 1"), "", "",
              uuid(4), uuid(5));
  dev1.saveTo(dev1Dir);

  // Save everything to disk
  mFs->save();

  // Create dialog
  AddComponentDialog dialog(*mWsDb, {}, {});
  QTreeView& catView =
      TestHelpers::getChild<QTreeView>(dialog, "treeCategories");
  QTreeWidget& cmpView =
      TestHelpers::getChild<QTreeWidget>(dialog, "treeComponents");
  QLabel& lblCmpName = TestHelpers::getChild<QLabel>(dialog, "lblCompName");
  QComboBox& cbxSymbVar =
      TestHelpers::getChild<QComboBox>(dialog, "cbxSymbVar");
  QLabel& lblDevName = TestHelpers::getChild<QLabel>(dialog, "lblDeviceName");

  // Select cat 2
  QModelIndex cat1Index = catView.model()->index(0, 0);
  EXPECT_EQ("cat 1", cat1Index.data().toString().toStdString());
  QModelIndex cat2Index = catView.model()->index(0, 0, cat1Index);
  EXPECT_EQ("cat 2", cat2Index.data().toString().toStdString());
  catView.setCurrentIndex(cat2Index);
  EXPECT_EQ(2, cmpView.model()->rowCount());

  // Select cmp 2
  QModelIndex cmp2Index = cmpView.model()->index(1, 0);
  EXPECT_EQ("cmp 2", cmp2Index.data().toString().toStdString());
  cmpView.setCurrentIndex(cmp2Index);
  EXPECT_EQ("cmp 2", lblCmpName.text().toStdString());
  EXPECT_EQ(2, cbxSymbVar.count());
  EXPECT_EQ("var 1", cbxSymbVar.currentText().toStdString());

  // Select symbvar 2
  cbxSymbVar.setCurrentIndex(1);
  EXPECT_EQ("var 2", cbxSymbVar.currentText().toStdString());

  // Check getters
  EXPECT_EQ(str(uuid(4)), str(dialog.getSelectedComponentUuid()));
  EXPECT_EQ(str(uuid(8)), str(dialog.getSelectedSymbVarUuid()));
  EXPECT_EQ(str(tl::nullopt), str(dialog.getSelectedDeviceUuid()));

  // Now select dev 1
  QModelIndex dev1Index = cmpView.model()->index(0, 0, cmp2Index);
  EXPECT_EQ("dev 1", dev1Index.data().toString().toStdString());
  cmpView.setCurrentIndex(dev1Index);
  EXPECT_EQ("dev 1 [pkg 1]", lblDevName.text().toStdString());

  // Check getters again
  EXPECT_EQ(str(uuid(4)), str(dialog.getSelectedComponentUuid()));
  EXPECT_EQ(str(uuid(8)), str(dialog.getSelectedSymbVarUuid()));
  EXPECT_EQ(str(uuid(6)), str(dialog.getSelectedDeviceUuid()));
}

TEST_F(AddComponentDialogTest, testSetNormOrder) {
  // - cat 1
  //   - cmp 1
  int catId = mWriter->addCategory<ComponentCategory>(
      0, toAbs("cat1"), uuid(1), version("0.1"), false, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(catId, "", ElementName("cat 1"),
                                             tl::nullopt, tl::nullopt);
  int cmpId = mWriter->addElement<Component>(0, toAbs(uuid(3).toStr()), uuid(3),
                                             version("0.1"), false);
  mWriter->addTranslation<Component>(cmpId, "", ElementName("cmp 1"),
                                     tl::nullopt, tl::nullopt);
  mWriter->addToCategory<Component>(cmpId, uuid(1));

  // Create component
  TransactionalDirectory cmp2Dir(mFs, uuid(3).toStr());
  Component cmp2(uuid(3), version("0.1"), "", ElementName("cmp 2"), "", "");
  auto cmp2SymbVar1 = std::make_shared<ComponentSymbolVariant>(
      uuid(4), "", ElementName("var 1"), "");
  cmp2.getSymbolVariants().append(cmp2SymbVar1);
  auto cmp2SymbVar2 = std::make_shared<ComponentSymbolVariant>(
      uuid(5), "NORM", ElementName("var 2"), "");
  cmp2.getSymbolVariants().append(cmp2SymbVar2);
  cmp2.saveTo(cmp2Dir);

  // Save everything to disk
  mFs->save();

  // Create dialog
  AddComponentDialog dialog(*mWsDb, {}, {"NORM"});
  QTreeView& catView =
      TestHelpers::getChild<QTreeView>(dialog, "treeCategories");
  QTreeWidget& cmpView =
      TestHelpers::getChild<QTreeWidget>(dialog, "treeComponents");
  QComboBox& cbxSymbVar =
      TestHelpers::getChild<QComboBox>(dialog, "cbxSymbVar");

  // Select cmp 1 and check selected symbol variant
  catView.setCurrentIndex(catView.model()->index(0, 0));
  cmpView.setCurrentIndex(cmpView.model()->index(0, 0));
  EXPECT_EQ("var 2 [NORM]", cbxSymbVar.currentText().toStdString());

  // Change norm order
  dialog.setNormOrder({});

  // Update selection and check selected symbol variant again
  catView.setCurrentIndex(QModelIndex());
  catView.setCurrentIndex(catView.model()->index(0, 0));
  cmpView.setCurrentIndex(cmpView.model()->index(0, 0));
  EXPECT_EQ("var 1", cbxSymbVar.currentText().toStdString());
}

TEST_F(AddComponentDialogTest, testSearch) {
  int cmpId = mWriter->addElement<Component>(0, toAbs("cmp1"), uuid(1),
                                             version("0.1"), false);
  mWriter->addTranslation<Component>(cmpId, "", ElementName("cmp 1"),
                                     tl::nullopt, "key 1");
  cmpId = mWriter->addElement<Component>(0, toAbs("cmp2"), uuid(2),
                                         version("0.1"), false);
  mWriter->addTranslation<Component>(cmpId, "", ElementName("cmp 2"),
                                     tl::nullopt, tl::nullopt);

  // Create dialog
  AddComponentDialog dialog(*mWsDb, {}, {});
  QLineEdit& edtSearch = TestHelpers::getChild<QLineEdit>(dialog, "edtSearch");
  QTreeWidget& cmpView =
      TestHelpers::getChild<QTreeWidget>(dialog, "treeComponents");

  // Search "cmp" -> 2 results
  edtSearch.setText("cmp");
  EXPECT_EQ(2, cmpView.model()->rowCount());
  EXPECT_EQ("cmp 1",
            cmpView.model()->index(0, 0).data().toString().toStdString());
  EXPECT_EQ("cmp 2",
            cmpView.model()->index(1, 0).data().toString().toStdString());

  // Search "foo" -> 0 results
  edtSearch.setText("foo");
  EXPECT_EQ(0, cmpView.model()->rowCount());

  // Search "key" -> 1 results
  edtSearch.setText("key");
  EXPECT_EQ(1, cmpView.model()->rowCount());
  EXPECT_EQ("cmp 1",
            cmpView.model()->index(0, 0).data().toString().toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
