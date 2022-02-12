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
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/sqlitedatabase.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacelibrarydbwriter.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class WorkspaceLibraryDbTest : public ::testing::Test {
protected:
  FilePath mWsDir;
  std::unique_ptr<WorkspaceLibraryDb> mWsDb;
  std::unique_ptr<SQLiteDatabase> mDb;
  std::unique_ptr<WorkspaceLibraryDbWriter> mWriter;

  WorkspaceLibraryDbTest() : mWsDir(FilePath::getRandomTempPath()) {
    FileUtils::makePath(mWsDir);
    mWsDb.reset(new WorkspaceLibraryDb(mWsDir));
    mDb.reset(new SQLiteDatabase(mWsDb->getFilePath()));
    mWriter.reset(new WorkspaceLibraryDbWriter(mWsDir, *mDb));
  }

  virtual ~WorkspaceLibraryDbTest() {
    QDir(mWsDir.toStr()).removeRecursively();
  }

  std::string str(const FilePath& fp) { return fp.toStr().toStdString(); }

  std::string str(const Uuid& uuid) { return uuid.toStr().toStdString(); }

  std::string str(const Version& version) {
    return version.toStr().toStdString();
  }

  std::string str(const QList<Uuid>& set) {
    QStringList s;
    foreach (const Uuid& uuid, set) { s.append(uuid.toStr()); }
    return s.join(", ").toStdString();
  }

  std::string str(const QSet<Uuid>& set) {
    return str(Toolbox::sortedQSet(set));
  }

  std::string str(const QMultiMap<Version, FilePath>& map) {
    QStringList s;
    for (auto it = map.constBegin(); it != map.constEnd(); it++) {
      s.append(it.key().toStr() % " -> " % it.value().toStr());
    }
    return s.join(", ").toStdString();
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
 *  Tests for getAll()
 ******************************************************************************/

TEST_F(WorkspaceLibraryDbTest, testGetAllEmptyDb) {
  EXPECT_EQ(0, mWsDb->getAll<Library>().count());
  EXPECT_EQ(0, mWsDb->getAll<ComponentCategory>().count());
  EXPECT_EQ(0, mWsDb->getAll<PackageCategory>().count());
  EXPECT_EQ(0, mWsDb->getAll<Symbol>().count());
  EXPECT_EQ(0, mWsDb->getAll<Package>().count());
  EXPECT_EQ(0, mWsDb->getAll<Component>().count());
  EXPECT_EQ(0, mWsDb->getAll<Device>().count());
}

TEST_F(WorkspaceLibraryDbTest, testGetAllEmptyDbWithUuid) {
  EXPECT_EQ(0, mWsDb->getAll<Library>(uuid()).count());
  EXPECT_EQ(0, mWsDb->getAll<ComponentCategory>(uuid()).count());
  EXPECT_EQ(0, mWsDb->getAll<PackageCategory>(uuid()).count());
  EXPECT_EQ(0, mWsDb->getAll<Symbol>(uuid()).count());
  EXPECT_EQ(0, mWsDb->getAll<Package>(uuid()).count());
  EXPECT_EQ(0, mWsDb->getAll<Component>(uuid()).count());
  EXPECT_EQ(0, mWsDb->getAll<Device>(uuid()).count());
}

TEST_F(WorkspaceLibraryDbTest, testGetAllEmptyDbWithLibrary) {
  FilePath lib = toAbs("lib");
  // Note: Library filter with getAll<Library> is not possible, thus not tested.
  EXPECT_EQ(0, mWsDb->getAll<ComponentCategory>(tl::nullopt, lib).count());
  EXPECT_EQ(0, mWsDb->getAll<PackageCategory>(tl::nullopt, lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Symbol>(tl::nullopt, lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Package>(tl::nullopt, lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Component>(tl::nullopt, lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Device>(tl::nullopt, lib).count());
}

TEST_F(WorkspaceLibraryDbTest, testGetAllEmptyDbWithUuidAndLibrary) {
  FilePath lib = toAbs("lib");
  // Note: Library filter with getAll<Library> is not possible, thus not tested.
  EXPECT_EQ(0, mWsDb->getAll<ComponentCategory>(uuid(), lib).count());
  EXPECT_EQ(0, mWsDb->getAll<PackageCategory>(uuid(), lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Symbol>(uuid(), lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Package>(uuid(), lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Component>(uuid(), lib).count());
  EXPECT_EQ(0, mWsDb->getAll<Device>(uuid(), lib).count());
}

TEST_F(WorkspaceLibraryDbTest, testGetAll) {
  for (int i = 0; i < 2; ++i) {
    QString number = QString::number(i + 1);
    mWriter->addLibrary(toAbs("lib" % number), uuid(), version("0.1." % number),
                        false, QByteArray());
    mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat" % number), uuid(),
                                            version("0.2." % number), false,
                                            tl::nullopt);
    mWriter->addCategory<PackageCategory>(0, toAbs("pkgcat" % number), uuid(),
                                          version("0.3." % number), false,
                                          tl::nullopt);
    mWriter->addElement<Symbol>(0, toAbs("sym" % number), uuid(),
                                version("0.4." % number), false);
    mWriter->addElement<Package>(0, toAbs("pkg" % number), uuid(),
                                 version("0.5." % number), false);
    mWriter->addElement<Component>(0, toAbs("cmp" % number), uuid(),
                                   version("0.6." % number), false);
    mWriter->addDevice(0, toAbs("dev" % number), uuid(),
                       version("0.7." % number), false, uuid(), uuid());
  }

  EXPECT_EQ(str({{version("0.1.1"), toAbs("lib1")},
                 {version("0.1.2"), toAbs("lib2")}}),
            str(mWsDb->getAll<Library>()));
  EXPECT_EQ(str({{version("0.2.1"), toAbs("cmpcat1")},
                 {version("0.2.2"), toAbs("cmpcat2")}}),
            str(mWsDb->getAll<ComponentCategory>()));
  EXPECT_EQ(str({{version("0.3.1"), toAbs("pkgcat1")},
                 {version("0.3.2"), toAbs("pkgcat2")}}),
            str(mWsDb->getAll<PackageCategory>()));
  EXPECT_EQ(str({{version("0.4.1"), toAbs("sym1")},
                 {version("0.4.2"), toAbs("sym2")}}),
            str(mWsDb->getAll<Symbol>()));
  EXPECT_EQ(str({{version("0.5.1"), toAbs("pkg1")},
                 {version("0.5.2"), toAbs("pkg2")}}),
            str(mWsDb->getAll<Package>()));
  EXPECT_EQ(str({{version("0.6.1"), toAbs("cmp1")},
                 {version("0.6.2"), toAbs("cmp2")}}),
            str(mWsDb->getAll<Component>()));
  EXPECT_EQ(str({{version("0.7.1"), toAbs("dev1")},
                 {version("0.7.2"), toAbs("dev2")}}),
            str(mWsDb->getAll<Device>()));
}

// Further tests only check with Symbol, since the implementation is the same
// for all library element types and the tests above have proven that each
// element type is generally working.

TEST_F(WorkspaceLibraryDbTest, testGetAllWithDuplicates) {
  int lib1 = mWriter->addLibrary(toAbs("lib1"), uuid(), version("1"), false,
                                 QByteArray());
  int lib2 = mWriter->addLibrary(toAbs("lib2"), uuid(), version("2"), false,
                                 QByteArray());
  mWriter->addElement<Symbol>(lib1, toAbs("sym1"), uuid(1), version("0.1"),
                              false);
  mWriter->addElement<Symbol>(lib1, toAbs("sym2"), uuid(2), version("0.2"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym3"), uuid(1), version("0.3"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym4"), uuid(2), version("0.2"),
                              false);

  EXPECT_EQ(str({{version("0.1"), toAbs("sym1")},
                 {version("0.2"), toAbs("sym2")},
                 {version("0.3"), toAbs("sym3")},
                 {version("0.2"), toAbs("sym4")}}),
            str(mWsDb->getAll<Symbol>()));
}

TEST_F(WorkspaceLibraryDbTest, testGetAllWithUuid) {
  int lib1 = mWriter->addLibrary(toAbs("lib1"), uuid(), version("1"), false,
                                 QByteArray());
  int lib2 = mWriter->addLibrary(toAbs("lib2"), uuid(), version("2"), false,
                                 QByteArray());
  mWriter->addElement<Symbol>(lib1, toAbs("sym1"), uuid(1), version("0.1"),
                              false);
  mWriter->addElement<Symbol>(lib1, toAbs("sym2"), uuid(2), version("0.2"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym3"), uuid(1), version("0.3"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym4"), uuid(2), version("0.2"),
                              false);

  EXPECT_EQ(
      str({{version("0.1"), toAbs("sym1")}, {version("0.3"), toAbs("sym3")}}),
      str(mWsDb->getAll<Symbol>(uuid(1))));
}

TEST_F(WorkspaceLibraryDbTest, testGetAllWithLibrary) {
  int lib1 = mWriter->addLibrary(toAbs("lib1"), uuid(), version("1"), false,
                                 QByteArray());
  int lib2 = mWriter->addLibrary(toAbs("lib2"), uuid(), version("2"), false,
                                 QByteArray());
  mWriter->addElement<Symbol>(lib1, toAbs("sym1"), uuid(1), version("0.1"),
                              false);
  mWriter->addElement<Symbol>(lib1, toAbs("sym2"), uuid(2), version("0.2"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym3"), uuid(1), version("0.3"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym4"), uuid(2), version("0.2"),
                              false);

  EXPECT_EQ(
      str({{version("0.3"), toAbs("sym3")}, {version("0.2"), toAbs("sym4")}}),
      str(mWsDb->getAll<Symbol>(tl::nullopt, toAbs("lib2"))));
}

TEST_F(WorkspaceLibraryDbTest, testGetAllWithUuidAndLibrary) {
  int lib1 = mWriter->addLibrary(toAbs("lib1"), uuid(), version("1"), false,
                                 QByteArray());
  int lib2 = mWriter->addLibrary(toAbs("lib2"), uuid(), version("2"), false,
                                 QByteArray());
  mWriter->addElement<Symbol>(lib1, toAbs("sym1"), uuid(1), version("0.1"),
                              false);
  mWriter->addElement<Symbol>(lib1, toAbs("sym2"), uuid(2), version("0.2"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym3"), uuid(1), version("0.3"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym4"), uuid(2), version("0.2"),
                              false);

  EXPECT_EQ(str({{version("0.3"), toAbs("sym3")}}),
            str(mWsDb->getAll<Symbol>(uuid(1), toAbs("lib2"))));
}

/*******************************************************************************
 *  Tests for getLatest()
 ******************************************************************************/

// Only very few, simple tests since the implementation is only a small,
// generic wrapper around getAll().

TEST_F(WorkspaceLibraryDbTest, testGetLatestEmptyDb) {
  EXPECT_FALSE(mWsDb->getLatest<Symbol>(uuid()).isValid());
}

TEST_F(WorkspaceLibraryDbTest, testGetLatest) {
  int lib1 = mWriter->addLibrary(toAbs("lib1"), uuid(), version("1"), false,
                                 QByteArray());
  int lib2 = mWriter->addLibrary(toAbs("lib2"), uuid(), version("2"), false,
                                 QByteArray());
  mWriter->addElement<Symbol>(lib1, toAbs("sym1"), uuid(0), version("0.1"),
                              false);
  mWriter->addElement<Symbol>(lib1, toAbs("sym2"), uuid(0), version("0.2"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym3"), uuid(0), version("0.3"),
                              false);
  mWriter->addElement<Symbol>(lib2, toAbs("sym4"), uuid(0), version("0.2"),
                              false);

  EXPECT_EQ(str(toAbs("sym3")), str(mWsDb->getLatest<Symbol>(uuid(0))));
}

/*******************************************************************************
 *  Tests for find()
 ******************************************************************************/

// Only tested with Symbol, since the implementation is shared across all
// element types.

TEST_F(WorkspaceLibraryDbTest, testFindEmptyDb) {
  EXPECT_EQ(str(QList<Uuid>{}), str(mWsDb->find<Symbol>("foo")));
}

TEST_F(WorkspaceLibraryDbTest, testFindEmptyKeyword) {
  int lib = mWriter->addLibrary(toAbs("lib"), uuid(), version("1"), false,
                                QByteArray());
  int sym = mWriter->addElement<Symbol>(lib, toAbs("sym1"), uuid(),
                                        version("0.1"), false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("some name"),
                                  "some desc", "some keywords");

  EXPECT_EQ(str(QList<Uuid>{}), str(mWsDb->find<Symbol>("foo")));
}

TEST_F(WorkspaceLibraryDbTest, testFind) {
  int lib = mWriter->addLibrary(toAbs("lib"), uuid(), version("1"), false,
                                QByteArray());
  int sym = mWriter->addElement<Symbol>(lib, toAbs("sym1"), uuid(1),
                                        version("0.1"), false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym1 name"),
                                  "the sym1 desc", "the sym1 keywords");
  sym = mWriter->addElement<Symbol>(lib, toAbs("sym2"), uuid(2), version("0.2"),
                                    false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym2 name"),
                                  "the sym2 desc", "the sym2 keywords");
  sym = mWriter->addElement<Symbol>(lib, toAbs("sym3"), uuid(3), version("0.3"),
                                    false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym3 name"),
                                  "the sym3 desc", "the sym3 keywords");

  EXPECT_EQ(str(QList<Uuid>{uuid(1), uuid(2), uuid(3)}),
            str(mWsDb->find<Symbol>("name")));
  EXPECT_EQ(str(QList<Uuid>{uuid(1)}), str(mWsDb->find<Symbol>("sym1 name")));
  EXPECT_EQ(str(QList<Uuid>{uuid(3)}),
            str(mWsDb->find<Symbol>("sym3 keywords")));

  // Descriptions are not taken into account to avoit way too verbose results!
  EXPECT_EQ(str(QList<Uuid>{}), str(mWsDb->find<Symbol>("sym2 desc")));
}

TEST_F(WorkspaceLibraryDbTest, testFindWithDuplicates) {
  int lib = mWriter->addLibrary(toAbs("lib"), uuid(), version("1"), false,
                                QByteArray());
  int sym = mWriter->addElement<Symbol>(lib, toAbs("sym1"), uuid(1),
                                        version("0.1"), false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym1 name"),
                                  "the sym1 desc", "the sym1 keywords");
  sym = mWriter->addElement<Symbol>(lib, toAbs("sym2"), uuid(1), version("0.2"),
                                    false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym2 name"),
                                  "the sym2 desc", "the sym2 keywords");
  sym = mWriter->addElement<Symbol>(lib, toAbs("sym3"), uuid(2), version("0.3"),
                                    false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym3 name"),
                                  "the sym3 desc", "the sym3 keywords");

  EXPECT_EQ(str(QList<Uuid>{uuid(1), uuid(2)}),
            str(mWsDb->find<Symbol>("name")));
  EXPECT_EQ(str(QList<Uuid>{uuid(1)}), str(mWsDb->find<Symbol>("sym1 name")));
}

TEST_F(WorkspaceLibraryDbTest, testFindWithMultipleTranslations) {
  int lib = mWriter->addLibrary(toAbs("lib"), uuid(), version("1"), false,
                                QByteArray());
  int sym = mWriter->addElement<Symbol>(lib, toAbs("sym1"), uuid(1),
                                        version("0.1"), false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym1 name"),
                                  "the sym1 desc", "the sym1 keywords");
  mWriter->addTranslation<Symbol>(
      sym, "en_US", ElementName("the sym1 en_US name"), "the sym1 en_US desc",
      "the sym1 en_US keywords");
  mWriter->addTranslation<Symbol>(
      sym, "de_DE", ElementName("the sym1 de_DE name"), "the sym1 de_DE desc",
      "the sym1 de_DE keywords");
  sym = mWriter->addElement<Symbol>(lib, toAbs("sym2"), uuid(2), version("0.2"),
                                    false);
  mWriter->addTranslation<Symbol>(sym, "", ElementName("the sym2 name"),
                                  "the sym2 desc", "the sym2 keywords");

  EXPECT_EQ(str(QList<Uuid>{uuid(1), uuid(2)}),
            str(mWsDb->find<Symbol>("name")));
  EXPECT_EQ(str(QList<Uuid>{uuid(1)}), str(mWsDb->find<Symbol>("sym1 name")));
  EXPECT_EQ(str(QList<Uuid>{uuid(1)}),
            str(mWsDb->find<Symbol>("sym1 en_US name")));
}

/*******************************************************************************
 *  Tests for getTranslations()
 ******************************************************************************/

template <typename ElementType>
static void testGetTr(WorkspaceLibraryDb& db, const FilePath& fp,
                      const QStringList& localeOrder, bool expectedReturnValue,
                      const QString& expectedName,
                      const QString& expectedDescription,
                      const QString& expectedKeywords) {
  QString name = "_default", description = "_default", keywords = "_default";
  const bool returnValue = db.getTranslations<ElementType>(
      fp, localeOrder, &name, &description, &keywords);
  EXPECT_EQ(expectedReturnValue, returnValue);
  EXPECT_EQ(expectedName.toStdString(), name.toStdString());
  EXPECT_EQ(expectedDescription.toStdString(), description.toStdString());
  EXPECT_EQ(expectedKeywords.toStdString(), keywords.toStdString());
}

TEST_F(WorkspaceLibraryDbTest, testGetTranslationsInexistent) {
  testGetTr<Library>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<ComponentCategory>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<PackageCategory>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Symbol>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Package>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Component>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Device>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
}

TEST_F(WorkspaceLibraryDbTest, testGetTranslationsEmpty) {
  int libId = mWriter->addLibrary(toAbs("fp"), uuid(), version("0.1"), false,
                                  QByteArray());
  mWriter->addCategory<ComponentCategory>(libId, toAbs("fp"), uuid(),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<PackageCategory>(libId, toAbs("fp"), uuid(),
                                        version("0.1"), false, tl::nullopt);
  mWriter->addElement<Symbol>(libId, toAbs("fp"), uuid(), version("0.1"),
                              false);
  mWriter->addElement<Package>(libId, toAbs("fp"), uuid(), version("0.1"),
                               false);
  mWriter->addElement<Component>(libId, toAbs("fp"), uuid(), version("0.1"),
                                 false);
  mWriter->addDevice(libId, toAbs("fp"), uuid(), version("0.1"), false, uuid(),
                     uuid());

  testGetTr<Library>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<ComponentCategory>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<PackageCategory>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Symbol>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Package>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Component>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
  testGetTr<Device>(*mWsDb, toAbs("fp"), {}, false, "", "", "");
}

TEST_F(WorkspaceLibraryDbTest, testGetTranslationsDefaultLocale) {
  int libId = mWriter->addLibrary(toAbs("fp"), uuid(), version("0.1"), false,
                                  QByteArray());
  mWriter->addTranslation<Library>(libId, "", ElementName("lib_n"), "lib_d",
                                   "lib_k");
  int id = mWriter->addCategory<ComponentCategory>(
      libId, toAbs("fp"), uuid(), version("0.1"), false, tl::nullopt);
  mWriter->addTranslation<ComponentCategory>(id, "", ElementName("cmpcat_n"),
                                             "cmpcat_d", "cmpcat_k");
  id = mWriter->addCategory<PackageCategory>(
      libId, toAbs("fp"), uuid(), version("0.1"), false, tl::nullopt);
  mWriter->addTranslation<PackageCategory>(id, "", ElementName("pkgcat_n"),
                                           "pkgcat_d", "pkgcat_k");
  id = mWriter->addElement<Symbol>(libId, toAbs("fp"), uuid(), version("0.1"),
                                   false);
  mWriter->addTranslation<Symbol>(id, "", ElementName("sym_n"), "sym_d",
                                  "sym_k");
  id = mWriter->addElement<Package>(libId, toAbs("fp"), uuid(), version("0.1"),
                                    false);
  mWriter->addTranslation<Package>(id, "", ElementName("pkg_n"), "pkg_d",
                                   "pkg_k");
  id = mWriter->addElement<Component>(libId, toAbs("fp"), uuid(),
                                      version("0.1"), false);
  mWriter->addTranslation<Component>(id, "", ElementName("cmp_n"), "cmp_d",
                                     "cmp_k");
  id = mWriter->addDevice(libId, toAbs("fp"), uuid(), version("0.1"), false,
                          uuid(), uuid());
  mWriter->addTranslation<Device>(id, "", ElementName("dev_n"), "dev_d",
                                  "dev_k");

  testGetTr<Library>(*mWsDb, toAbs("fp"), {}, true, "lib_n", "lib_d", "lib_k");
  testGetTr<ComponentCategory>(*mWsDb, toAbs("fp"), {}, true, "cmpcat_n",
                               "cmpcat_d", "cmpcat_k");
  testGetTr<PackageCategory>(*mWsDb, toAbs("fp"), {}, true, "pkgcat_n",
                             "pkgcat_d", "pkgcat_k");
  testGetTr<Symbol>(*mWsDb, toAbs("fp"), {}, true, "sym_n", "sym_d", "sym_k");
  testGetTr<Package>(*mWsDb, toAbs("fp"), {}, true, "pkg_n", "pkg_d", "pkg_k");
  testGetTr<Component>(*mWsDb, toAbs("fp"), {}, true, "cmp_n", "cmp_d",
                       "cmp_k");
  testGetTr<Device>(*mWsDb, toAbs("fp"), {}, true, "dev_n", "dev_d", "dev_k");
}

// Further tests only check with Symbol, since the implementation is the same
// for all library element types and the tests above have proven that each
// element type is generally working.

TEST_F(WorkspaceLibraryDbTest, testGetTranslationsDefaultWithOrder) {
  int id = mWriter->addElement<Symbol>(0, toAbs("fp"), uuid(), version("0.1"),
                                       false);
  mWriter->addTranslation<Symbol>(id, "", ElementName("_n"), "_d", "_k");

  testGetTr<Symbol>(*mWsDb, toAbs("fp"), QStringList{"en_US", "zh_CN", "de_DE"},
                    true, "_n", "_d", "_k");
}

TEST_F(WorkspaceLibraryDbTest, testGetTranslationsMultipleWithoutOrder) {
  int id = mWriter->addElement<Symbol>(0, toAbs("fp"), uuid(), version("0.1"),
                                       false);
  mWriter->addTranslation<Symbol>(id, "de_DE", tl::nullopt, "de_d",
                                  tl::nullopt);
  mWriter->addTranslation<Symbol>(id, "", ElementName("_n"), "_d", "_k");
  mWriter->addTranslation<Symbol>(id, "en_US", ElementName("en_n"), tl::nullopt,
                                  tl::nullopt);
  mWriter->addTranslation<Symbol>(id, "it_IT", ElementName("it_n"), "it_d",
                                  "it_k");

  testGetTr<Symbol>(*mWsDb, toAbs("fp"), {}, true, "_n", "_d", "_k");
}

TEST_F(WorkspaceLibraryDbTest, testGetTranslationsMultipleWithOrder) {
  int id = mWriter->addElement<Symbol>(0, toAbs("fp"), uuid(), version("0.1"),
                                       false);
  mWriter->addTranslation<Symbol>(id, "de_DE", tl::nullopt, "de_d",
                                  tl::nullopt);
  mWriter->addTranslation<Symbol>(id, "", tl::nullopt, "_d", "_k");
  mWriter->addTranslation<Symbol>(id, "en_US", ElementName("en_n"), tl::nullopt,
                                  tl::nullopt);
  mWriter->addTranslation<Symbol>(id, "it_IT", ElementName("it_n"), "it_d",
                                  "it_k");

  testGetTr<Symbol>(*mWsDb, toAbs("fp"), QStringList{"en_US", "zh_CN", "de_DE"},
                    true, "en_n", "de_d", "_k");
}

TEST_F(WorkspaceLibraryDbTest, testGetTranslationsNullptr) {
  int id = mWriter->addElement<Symbol>(0, toAbs("fp"), uuid(), version("0.1"),
                                       false);
  mWriter->addTranslation<Symbol>(id, "", ElementName("_n"), "_d", "_k");

  QStringList localeOrder{"en_US", "zh_CN", "de_DE"};
  QString name = "_default", description = "_default", keywords = "_default";
  mWsDb->getTranslations<Symbol>(toAbs("fp"), localeOrder);
  mWsDb->getTranslations<Symbol>(toAbs("fp"), localeOrder, &name, nullptr,
                                 nullptr);
  mWsDb->getTranslations<Symbol>(toAbs("fp"), localeOrder, nullptr,
                                 &description, nullptr);
  mWsDb->getTranslations<Symbol>(toAbs("fp"), localeOrder, nullptr, nullptr,
                                 &keywords);
  EXPECT_EQ("_n", name.toStdString());
  EXPECT_EQ("_d", description.toStdString());
  EXPECT_EQ("_k", keywords.toStdString());
}

/*******************************************************************************
 *  Tests for getMetadata()
 ******************************************************************************/

template <typename ElementType>
static void testGetMetadata(WorkspaceLibraryDb& db, const FilePath& fp,
                            bool expRetVal, const tl::optional<Uuid>& expUuid,
                            const tl::optional<Version>& expVersion,
                            const tl::optional<bool>& expDeprecated) {
  Uuid retUuid = Uuid::createRandom();
  Version retVersion = Version::fromString("1");
  bool retDeprecated = false;
  const bool retVal =
      db.getMetadata<ElementType>(fp, &retUuid, &retVersion, &retDeprecated);
  EXPECT_EQ(expRetVal, retVal);
  if (expUuid) {
    EXPECT_EQ(expUuid->toStr().toStdString(), retUuid.toStr().toStdString());
  }
  if (expVersion) {
    EXPECT_EQ(expVersion->toStr().toStdString(),
              retVersion.toStr().toStdString());
  }
  if (expDeprecated) {
    EXPECT_EQ(*expDeprecated, retDeprecated);
  }
}

TEST_F(WorkspaceLibraryDbTest, testGetMetadataInexistent) {
  FilePath fp = toAbs("fp");
  testGetMetadata<Library>(*mWsDb, fp, false, tl::nullopt, tl::nullopt,
                           tl::nullopt);
  testGetMetadata<ComponentCategory>(*mWsDb, fp, false, tl::nullopt,
                                     tl::nullopt, tl::nullopt);
  testGetMetadata<PackageCategory>(*mWsDb, fp, false, tl::nullopt, tl::nullopt,
                                   tl::nullopt);
  testGetMetadata<Symbol>(*mWsDb, fp, false, tl::nullopt, tl::nullopt,
                          tl::nullopt);
  testGetMetadata<Package>(*mWsDb, fp, false, tl::nullopt, tl::nullopt,
                           tl::nullopt);
  testGetMetadata<Component>(*mWsDb, fp, false, tl::nullopt, tl::nullopt,
                             tl::nullopt);
  testGetMetadata<Device>(*mWsDb, fp, false, tl::nullopt, tl::nullopt,
                          tl::nullopt);
}

TEST_F(WorkspaceLibraryDbTest, testGetMetadata) {
  FilePath fp = toAbs("fp");
  int libId =
      mWriter->addLibrary(fp, uuid(1), version("1.1"), false, QByteArray());
  mWriter->addCategory<ComponentCategory>(libId, fp, uuid(2), version("2.2"),
                                          true, tl::nullopt);
  mWriter->addCategory<PackageCategory>(libId, fp, uuid(3), version("3.3"),
                                        false, tl::nullopt);
  mWriter->addElement<Symbol>(libId, fp, uuid(4), version("4.4"), true);
  mWriter->addElement<Package>(libId, fp, uuid(5), version("5.5"), false);
  mWriter->addElement<Component>(libId, fp, uuid(6), version("6.6"), true);
  mWriter->addDevice(libId, fp, uuid(7), version("7.7"), false, uuid(), uuid());

  testGetMetadata<Library>(*mWsDb, fp, true, uuid(1), version("1.1"), false);
  testGetMetadata<ComponentCategory>(*mWsDb, fp, true, uuid(2), version("2.2"),
                                     true);
  testGetMetadata<PackageCategory>(*mWsDb, fp, true, uuid(3), version("3.3"),
                                   false);
  testGetMetadata<Symbol>(*mWsDb, fp, true, uuid(4), version("4.4"), true);
  testGetMetadata<Package>(*mWsDb, fp, true, uuid(5), version("5.5"), false);
  testGetMetadata<Component>(*mWsDb, fp, true, uuid(6), version("6.6"), true);
  testGetMetadata<Device>(*mWsDb, fp, true, uuid(7), version("7.7"), false);
}

// Further tests only check with Symbol, since the implementation is the same
// for all library element types and the tests above have proven that each
// element type is generally working.

TEST_F(WorkspaceLibraryDbTest, testGetMetadataNullptr) {
  FilePath fp = toAbs("fp");
  mWriter->addElement<Symbol>(0, fp, uuid(1), version("1.1"), true);

  Uuid retUuid = Uuid::createRandom();
  Version retVersion = version("1");
  bool retDeprecated = false;
  mWsDb->getMetadata<Symbol>(fp);
  mWsDb->getMetadata<Symbol>(fp, &retUuid);
  mWsDb->getMetadata<Symbol>(fp, nullptr, &retVersion);
  mWsDb->getMetadata<Symbol>(fp, nullptr, nullptr, &retDeprecated);
  EXPECT_EQ(str(uuid(1)), str(retUuid));
  EXPECT_EQ("1.1", str(retVersion));
  EXPECT_TRUE(retDeprecated);
}

/*******************************************************************************
 *  Tests for getLibraryMetadata()
 ******************************************************************************/

TEST_F(WorkspaceLibraryDbTest, testGetLibraryMetadataInexistent) {
  QPixmap icon;
  EXPECT_FALSE(mWsDb->getLibraryMetadata(toAbs("fp"), &icon));
  EXPECT_TRUE(icon.isNull());
}

TEST_F(WorkspaceLibraryDbTest, testGetLibraryMetadataNoIcon) {
  FilePath fp = toAbs("fp");
  mWriter->addLibrary(fp, uuid(), version("1.1"), false, QByteArray());

  QPixmap icon;
  EXPECT_TRUE(mWsDb->getLibraryMetadata(fp, &icon));
  EXPECT_TRUE(icon.isNull());
}

TEST_F(WorkspaceLibraryDbTest, testGetLibraryMetadataNullptr) {
  FilePath fp = toAbs("fp");
  mWriter->addLibrary(fp, uuid(), version("1.1"), false, QByteArray());

  EXPECT_TRUE(mWsDb->getLibraryMetadata(fp));
}

/*******************************************************************************
 *  Tests for getCategoryMetadata()
 ******************************************************************************/

TEST_F(WorkspaceLibraryDbTest, testGetCategoryMetadataEmptyDb) {
  tl::optional<Uuid> parent;
  EXPECT_FALSE(
      mWsDb->getCategoryMetadata<ComponentCategory>(toAbs("fp"), &parent));
  EXPECT_FALSE(
      mWsDb->getCategoryMetadata<PackageCategory>(toAbs("fp"), &parent));
}

TEST_F(WorkspaceLibraryDbTest, testGetCategoryMetadataInexistent) {
  int libId = mWriter->addLibrary(toAbs("fp"), uuid(), version("1.1"), false,
                                  QByteArray());
  mWriter->addCategory<ComponentCategory>(libId, toAbs("fp"), uuid(),
                                          version("2.2"), false, tl::nullopt);
  mWriter->addCategory<PackageCategory>(libId, toAbs("fp"), uuid(),
                                        version("3.3"), false, tl::nullopt);

  tl::optional<Uuid> parent;
  EXPECT_FALSE(
      mWsDb->getCategoryMetadata<ComponentCategory>(toAbs("foo"), &parent));
  EXPECT_FALSE(
      mWsDb->getCategoryMetadata<PackageCategory>(toAbs("foo"), &parent));
}

TEST_F(WorkspaceLibraryDbTest, testGetCategoryMetadata) {
  int libId = mWriter->addLibrary(toAbs("fp"), uuid(), version("1.1"), false,
                                  QByteArray());
  mWriter->addCategory<ComponentCategory>(libId, toAbs("fp1"), uuid(1),
                                          version("2.2"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(libId, toAbs("fp2"), uuid(2),
                                          version("3.3"), false, uuid(1));
  mWriter->addCategory<PackageCategory>(libId, toAbs("fp3"), uuid(2),
                                        version("4.4"), false, tl::nullopt);
  mWriter->addCategory<PackageCategory>(libId, toAbs("fp4"), uuid(1),
                                        version("5.5"), false, uuid(2));

  tl::optional<Uuid> parent;

  EXPECT_TRUE(
      mWsDb->getCategoryMetadata<ComponentCategory>(toAbs("fp1"), &parent));
  EXPECT_FALSE(parent.has_value());

  EXPECT_TRUE(
      mWsDb->getCategoryMetadata<ComponentCategory>(toAbs("fp2"), &parent));
  EXPECT_EQ(uuid(1), parent);

  EXPECT_TRUE(
      mWsDb->getCategoryMetadata<PackageCategory>(toAbs("fp3"), &parent));
  EXPECT_FALSE(parent.has_value());

  EXPECT_TRUE(
      mWsDb->getCategoryMetadata<PackageCategory>(toAbs("fp4"), &parent));
  EXPECT_EQ(uuid(2), parent);
}

TEST_F(WorkspaceLibraryDbTest, testGetCategoryMetadataNullptr) {
  int libId = mWriter->addLibrary(toAbs("fp"), uuid(), version("1.1"), false,
                                  QByteArray());
  mWriter->addCategory<ComponentCategory>(libId, toAbs("fp1"), uuid(1),
                                          version("2.2"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(libId, toAbs("fp2"), uuid(2),
                                          version("3.3"), false, uuid(1));

  tl::optional<Uuid> parent;

  EXPECT_TRUE(mWsDb->getCategoryMetadata<ComponentCategory>(toAbs("fp2")));
  EXPECT_TRUE(
      mWsDb->getCategoryMetadata<ComponentCategory>(toAbs("fp2"), &parent));
  EXPECT_EQ(uuid(1), parent);
}

/*******************************************************************************
 *  Tests for getDeviceMetadata()
 ******************************************************************************/

TEST_F(WorkspaceLibraryDbTest, testGetDeviceMetadataInexistent) {
  Uuid cmpUuid = Uuid::createRandom();
  Uuid pkgUuid = Uuid::createRandom();
  EXPECT_FALSE(mWsDb->getDeviceMetadata(toAbs("fp"), &cmpUuid, &pkgUuid));
}

TEST_F(WorkspaceLibraryDbTest, testGetDeviceMetadata) {
  FilePath fp = toAbs("fp");
  mWriter->addDevice(0, fp, uuid(), version("1.1"), false, uuid(1), uuid(2));

  Uuid cmpUuid = Uuid::createRandom();
  Uuid pkgUuid = Uuid::createRandom();
  EXPECT_TRUE(mWsDb->getDeviceMetadata(fp, &cmpUuid, &pkgUuid));
  EXPECT_EQ(str(uuid(1)), str(cmpUuid));
  EXPECT_EQ(str(uuid(2)), str(pkgUuid));
}

TEST_F(WorkspaceLibraryDbTest, testGetDeviceMetadataNullptr) {
  FilePath fp = toAbs("fp");
  mWriter->addDevice(0, fp, uuid(), version("1.1"), false, uuid(1), uuid(2));

  Uuid cmpUuid = Uuid::createRandom();
  Uuid pkgUuid = Uuid::createRandom();
  EXPECT_TRUE(mWsDb->getDeviceMetadata(fp));
  EXPECT_TRUE(mWsDb->getDeviceMetadata(fp, &cmpUuid, nullptr));
  EXPECT_TRUE(mWsDb->getDeviceMetadata(fp, nullptr, &pkgUuid));
  EXPECT_EQ(str(uuid(1)), str(cmpUuid));
  EXPECT_EQ(str(uuid(2)), str(pkgUuid));
}

/*******************************************************************************
 *  Tests for getChilds()
 ******************************************************************************/

TEST_F(WorkspaceLibraryDbTest, testGetChildsEmptyDb) {
  EXPECT_EQ(str(QSet<Uuid>{}),
            str(mWsDb->getChilds<ComponentCategory>(tl::nullopt)));
  EXPECT_EQ(str(QSet<Uuid>{}),
            str(mWsDb->getChilds<PackageCategory>(tl::nullopt)));
}

TEST_F(WorkspaceLibraryDbTest, testGetChildsInexistent) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat"), uuid(1),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<PackageCategory>(0, toAbs("pkgcat"), uuid(2),
                                        version("0.1"), false, tl::nullopt);

  EXPECT_EQ(str(QSet<Uuid>{}),
            str(mWsDb->getChilds<ComponentCategory>(uuid(2))));
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getChilds<PackageCategory>(uuid(1))));
}

TEST_F(WorkspaceLibraryDbTest, testGetChildsInvalidWithUuid) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat"), uuid(1),
                                          version("0.1"), false, uuid(2));
  mWriter->addCategory<PackageCategory>(0, toAbs("pkgcat"), uuid(3),
                                        version("0.1"), false, uuid(4));

  EXPECT_EQ(str(QSet<Uuid>{uuid(1)}),
            str(mWsDb->getChilds<ComponentCategory>(uuid(2))));
  EXPECT_EQ(str(QSet<Uuid>{uuid(3)}),
            str(mWsDb->getChilds<PackageCategory>(uuid(4))));
}

TEST_F(WorkspaceLibraryDbTest, testGetChildsInvalidWithoutUuid) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat"), uuid(1),
                                          version("0.1"), false, uuid(2));
  mWriter->addCategory<PackageCategory>(0, toAbs("pkgcat"), uuid(3),
                                        version("0.1"), false, uuid(4));

  EXPECT_EQ(str(QSet<Uuid>{uuid(1)}),
            str(mWsDb->getChilds<ComponentCategory>(tl::nullopt)));
  EXPECT_EQ(str(QSet<Uuid>{uuid(3)}),
            str(mWsDb->getChilds<PackageCategory>(tl::nullopt)));
}

// Further tests only check with ComponentCategory, since the implementation
// is the same for PackageCategory and the tests above have proven that each
// element type is generally working.

TEST_F(WorkspaceLibraryDbTest, testGetChildsDuplicatesWithUuid) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat1"), uuid(1),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat2"), uuid(2),
                                          version("0.1"), false, uuid(1));
  mWriter->addCategory<ComponentCategory>(1, toAbs("cmpcat3"), uuid(1),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(1, toAbs("cmpcat4"), uuid(2),
                                          version("0.1"), false, uuid(1));

  EXPECT_EQ(str(QSet<Uuid>{uuid(2)}),
            str(mWsDb->getChilds<ComponentCategory>(uuid(1))));
}

TEST_F(WorkspaceLibraryDbTest, testGetChildsDuplicatesWithoutUuid) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat1"), uuid(1),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat2"), uuid(2),
                                          version("0.1"), false, uuid(1));
  mWriter->addCategory<ComponentCategory>(1, toAbs("cmpcat3"), uuid(1),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(1, toAbs("cmpcat4"), uuid(2),
                                          version("0.1"), false, uuid(1));

  EXPECT_EQ(str(QSet<Uuid>{uuid(1)}),
            str(mWsDb->getChilds<ComponentCategory>(tl::nullopt)));
}

/*******************************************************************************
 *  Tests for getByCategory()
 ******************************************************************************/

TEST_F(WorkspaceLibraryDbTest, testGetByCategoryEmptyDb) {
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getByCategory<Symbol>(tl::nullopt)));
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getByCategory<Package>(tl::nullopt)));
  EXPECT_EQ(str(QSet<Uuid>{}),
            str(mWsDb->getByCategory<Component>(tl::nullopt)));
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getByCategory<Device>(tl::nullopt)));
}

TEST_F(WorkspaceLibraryDbTest, testGetByCategoryInexistent) {
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getByCategory<Symbol>(uuid())));
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getByCategory<Package>(uuid())));
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getByCategory<Component>(uuid())));
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getByCategory<Device>(uuid())));
}

TEST_F(WorkspaceLibraryDbTest, testGetByCategory) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat"), uuid(1),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<PackageCategory>(0, toAbs("pkgcat"), uuid(2),
                                        version("0.1"), false, tl::nullopt);
  int sym = mWriter->addElement<Symbol>(0, toAbs("sym"), uuid(3),
                                        version("0.1"), false);
  mWriter->addToCategory<Symbol>(sym, uuid(1));
  int pkg = mWriter->addElement<Package>(0, toAbs("pkg"), uuid(4),
                                         version("0.1"), false);
  mWriter->addToCategory<Package>(pkg, uuid(2));
  int cmp = mWriter->addElement<Component>(0, toAbs("cmp"), uuid(5),
                                           version("0.1"), false);
  mWriter->addToCategory<Component>(cmp, uuid(1));
  int dev = mWriter->addDevice(0, toAbs("dev"), uuid(6), version("0.1"), false,
                               uuid(), uuid());
  mWriter->addToCategory<Device>(dev, uuid(1));

  EXPECT_EQ(str(QSet<Uuid>{uuid(3)}),
            str(mWsDb->getByCategory<Symbol>(uuid(1))));
  EXPECT_EQ(str(QSet<Uuid>{uuid(4)}),
            str(mWsDb->getByCategory<Package>(uuid(2))));
  EXPECT_EQ(str(QSet<Uuid>{uuid(5)}),
            str(mWsDb->getByCategory<Component>(uuid(1))));
  EXPECT_EQ(str(QSet<Uuid>{uuid(6)}),
            str(mWsDb->getByCategory<Device>(uuid(1))));
}

// Further tests only check with Component, since the implementation is the
// same for all library element types and the tests above have proven that
// each element type is generally working.

TEST_F(WorkspaceLibraryDbTest, testGetByCategoryInvalidParent) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat"), uuid(1),
                                          version("0.1"), false, uuid(2));
  int cmp = mWriter->addElement<Component>(0, toAbs("fp"), uuid(3),
                                           version("0.1"), false);
  mWriter->addToCategory<Component>(cmp, uuid(1));

  // The category "uuid(1)" does not have a valid parent, but it will still
  // be listed in category trees as a root category. So its contained elements
  // shall be listed as usual, not in the "without category" node.
  EXPECT_EQ(str(QSet<Uuid>{uuid(3)}),
            str(mWsDb->getByCategory<Component>(uuid(1))));
  EXPECT_EQ(str(QSet<Uuid>{}),
            str(mWsDb->getByCategory<Component>(tl::nullopt)));
}

TEST_F(WorkspaceLibraryDbTest, testGetByCategoryEndlessRecursion) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat1"), uuid(1),
                                          version("0.1"), false, uuid(2));
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat2"), uuid(2),
                                          version("0.1"), false, uuid(1));
  int cmp = mWriter->addElement<Component>(0, toAbs("fp"), uuid(3),
                                           version("0.1"), false);
  mWriter->addToCategory<Component>(cmp, uuid(1));

  // None of the categories will be shown in the category tree, which is not
  // ideal but also not a big problem since endless recursion is not really
  // a real situation. However, the test should verify that nothing strange
  // happens here.
  EXPECT_EQ(str(QSet<Uuid>{uuid(3)}),
            str(mWsDb->getByCategory<Component>(uuid(1))));
  EXPECT_EQ(str(QSet<Uuid>{}),
            str(mWsDb->getByCategory<Component>(tl::nullopt)));
}

TEST_F(WorkspaceLibraryDbTest, testGetByCategoryDuplicates) {
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat1"), uuid(2),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(0, toAbs("cmpcat2"), uuid(1),
                                          version("0.1"), false, uuid(2));
  mWriter->addCategory<ComponentCategory>(1, toAbs("cmpcat3"), uuid(2),
                                          version("0.1"), false, tl::nullopt);
  mWriter->addCategory<ComponentCategory>(1, toAbs("cmpcat4"), uuid(1),
                                          version("0.1"), false, uuid(2));
  int cmp1 = mWriter->addElement<Component>(0, toAbs("cmp1"), uuid(3),
                                            version("0.1"), false);
  mWriter->addToCategory<Component>(cmp1, uuid(1));
  int cmp2 = mWriter->addElement<Component>(0, toAbs("cmp2"), uuid(3),
                                            version("0.1"), false);
  mWriter->addToCategory<Component>(cmp2, uuid(1));

  EXPECT_EQ(str(QSet<Uuid>{uuid(3)}),
            str(mWsDb->getByCategory<Component>(uuid(1))));
  EXPECT_EQ(str(QSet<Uuid>{}),
            str(mWsDb->getByCategory<Component>(tl::nullopt)));
}

/*******************************************************************************
 *  Tests for getComponentDevices()
 ******************************************************************************/

TEST_F(WorkspaceLibraryDbTest, testGetComponentDevicesEmptyDb) {
  EXPECT_EQ(str(QSet<Uuid>{}), str(mWsDb->getComponentDevices(uuid())));
}

TEST_F(WorkspaceLibraryDbTest, testGetComponentDevices) {
  mWriter->addDevice(0, toAbs("dev1"), uuid(1), version("0.1"), false, uuid(0),
                     uuid());
  mWriter->addDevice(0, toAbs("dev2"), uuid(2), version("0.1"), false, uuid(0),
                     uuid());
  mWriter->addDevice(0, toAbs("dev3"), uuid(3), version("0.1"), false, uuid(),
                     uuid());

  EXPECT_EQ(str(QSet<Uuid>{uuid(1), uuid(2)}),
            str(mWsDb->getComponentDevices(uuid(0))));
}

TEST_F(WorkspaceLibraryDbTest, testGetComponentDevicesDuplicates) {
  mWriter->addDevice(0, toAbs("dev1"), uuid(1), version("0.1"), false, uuid(0),
                     uuid());
  mWriter->addDevice(1, toAbs("dev2"), uuid(1), version("0.1"), false, uuid(0),
                     uuid());
  mWriter->addDevice(1, toAbs("dev3"), uuid(2), version("0.1"), false, uuid(),
                     uuid());

  EXPECT_EQ(str(QSet<Uuid>{uuid(1)}), str(mWsDb->getComponentDevices(uuid(0))));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
