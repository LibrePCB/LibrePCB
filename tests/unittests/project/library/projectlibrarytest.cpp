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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gtest/gtest.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/library/projectlibrary.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ProjectLibraryTest : public ::testing::Test {
protected:
  FilePath                        mTempDir;
  FilePath                        mLibDir;
  QFileInfo                       mExistingSymbolFile;
  qint64                          mExistingSymbolCreationSize;
  QScopedPointer<library::Symbol> mNewSymbol;
  QFileInfo                       mNewSymbolFile;

  ProjectLibraryTest() {
    mTempDir = FilePath::getRandomTempPath();
    mLibDir  = mTempDir.getPathTo("project library test");

    // create symbol inside project library
    library::Symbol sym(Uuid::createRandom(), Version::fromString("1"), "",
                        ElementName("Existing Symbol"), "", "");
    sym.saveIntoParentDirectory(mLibDir.getPathTo("sym"));
    mExistingSymbolFile = QFileInfo(
        mLibDir
            .getPathTo(QString("sym/%1/symbol.lp").arg(sym.getUuid().toStr()))
            .toStr());
    modifyExistingSymbol();  // modify file to detect when it gets overwritten

    // create symbol outside the project library (emulating workspace library)
    mNewSymbol.reset(new library::Symbol(Uuid::createRandom(),
                                         Version::fromString("1"), "",
                                         ElementName("New Symbol"), "", ""));
    mNewSymbol->saveIntoParentDirectory(mTempDir);
    mNewSymbolFile =
        mLibDir
            .getPathTo(
                QString("sym/%1/symbol.lp").arg(mNewSymbol->getUuid().toStr()))
            .toStr();

    // disable caching to get correct results
    mExistingSymbolFile.setCaching(false);
    mNewSymbolFile.setCaching(false);
  }

  virtual ~ProjectLibraryTest() { QDir(mTempDir.toStr()).removeRecursively(); }

  library::Symbol* getFirstSymbol(ProjectLibrary& lib) {
    if (lib.getSymbols().isEmpty()) throw LogicError(__FILE__, __LINE__);
    return lib.getSymbols().values().first();
  }

  void save(ProjectLibrary& lib, bool toOriginal) {
    QStringList errors;
    if (!lib.save(toOriginal, errors)) {
      throw RuntimeError(__FILE__, __LINE__, errors.join("\n"));
    }
  }

  void saveToTemporary(ProjectLibrary& lib) { save(lib, false); }

  void saveToOriginal(ProjectLibrary& lib) { save(lib, true); }

  void modifyExistingSymbol() {
    FilePath fp(mExistingSymbolFile.absoluteFilePath());
    FileUtils::writeFile(fp, FileUtils::readFile(fp).append(" "));
    mExistingSymbolCreationSize = mExistingSymbolFile.size();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ProjectLibraryTest, testLoadSymbol) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testAddSymbol) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.addSymbol(*mNewSymbol.take());
    EXPECT_EQ(2, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.dir().exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testAddSymbol_SaveToTemporary) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.addSymbol(*mNewSymbol.take());
    saveToTemporary(lib);
    EXPECT_EQ(2, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testAddSymbol_SaveToOriginal) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.addSymbol(*mNewSymbol.take());
    saveToTemporary(lib);
    saveToOriginal(lib);
    EXPECT_EQ(2, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_TRUE(mNewSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_TRUE(mNewSymbolFile.exists());
  EXPECT_NE(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // upgraded!
}

TEST_F(ProjectLibraryTest, testRestoreBackup) {
  // create backup
  ProjectLibrary lib(mLibDir, false, false);
  lib.addSymbol(*mNewSymbol.take());
  saveToTemporary(lib);
  EXPECT_EQ(2, lib.getSymbols().count());
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());

  {
    // restore backup
    ProjectLibrary lib2(mLibDir, true, false);
    EXPECT_EQ(2, lib2.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());

    // save to original
    saveToTemporary(lib2);
    saveToOriginal(lib2);
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_TRUE(mNewSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_TRUE(mNewSymbolFile.exists());
}

TEST_F(ProjectLibraryTest, testAddRemoveSymbol) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.addSymbol(*mNewSymbol);
    lib.removeSymbol(*mNewSymbol.take());
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.dir().exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testAddRemoveSymbol_SaveToTemporary) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.addSymbol(*mNewSymbol);
    lib.removeSymbol(*mNewSymbol.take());
    saveToTemporary(lib);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.dir().exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testAddRemoveSymbol_SaveToOriginal) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.addSymbol(*mNewSymbol);
    lib.removeSymbol(*mNewSymbol.take());
    saveToTemporary(lib);
    saveToOriginal(lib);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.dir().exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
  EXPECT_NE(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // upgraded!
}

TEST_F(ProjectLibraryTest, testRemoveSymbol) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.removeSymbol(*getFirstSymbol(lib));
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testRemoveSymbol_SaveToTemporary) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.removeSymbol(*getFirstSymbol(lib));
    saveToTemporary(lib);
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testRemoveSymbol_SaveToOriginal) {
  {
    ProjectLibrary lib(mLibDir, false, false);
    lib.removeSymbol(*getFirstSymbol(lib));
    saveToTemporary(lib);
    saveToOriginal(lib);
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_FALSE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mExistingSymbolFile.dir().exists());
  }
  EXPECT_FALSE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mExistingSymbolFile.dir().exists());
}

TEST_F(ProjectLibraryTest, testRemoveAddSymbol) {
  {
    ProjectLibrary   lib(mLibDir, false, false);
    library::Symbol* sym = getFirstSymbol(lib);
    lib.removeSymbol(*sym);
    lib.addSymbol(*sym);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testRemoveAddSymbol_SaveToTemporary) {
  {
    ProjectLibrary   lib(mLibDir, false, false);
    library::Symbol* sym = getFirstSymbol(lib);
    lib.removeSymbol(*sym);
    lib.addSymbol(*sym);
    saveToTemporary(lib);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

TEST_F(ProjectLibraryTest, testRemoveAddSymbol_SaveToOriginal) {
  {
    ProjectLibrary   lib(mLibDir, false, false);
    library::Symbol* sym = getFirstSymbol(lib);
    lib.removeSymbol(*sym);
    lib.addSymbol(*sym);
    saveToOriginal(lib);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_NE(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // upgraded!
}

TEST_F(ProjectLibraryTest,
       testRemoveSymbol_SaveToTemporary_AddNewSymbol_SaveToOriginal) {
  {
    ProjectLibrary lib(mLibDir, false, false);

    lib.removeSymbol(*getFirstSymbol(lib));
    saveToTemporary(lib);
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());

    lib.addSymbol(*mNewSymbol.take());
    saveToTemporary(lib);
    saveToOriginal(lib);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_FALSE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mExistingSymbolFile.dir().exists());
    EXPECT_TRUE(mNewSymbolFile.exists());
  }
  EXPECT_FALSE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mExistingSymbolFile.dir().exists());
  EXPECT_TRUE(mNewSymbolFile.exists());
}

TEST_F(
    ProjectLibraryTest,
    testAddNewSymbol_SaveToTemporary_RemoveSymbol_SaveToOriginal_AddSymbol_SaveToTemporary) {
  {
    ProjectLibrary   lib(mLibDir, false, false);
    library::Symbol* sym = getFirstSymbol(lib);

    lib.addSymbol(*mNewSymbol.take());
    saveToTemporary(lib);
    EXPECT_EQ(2, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());

    lib.removeSymbol(*sym);
    saveToTemporary(lib);
    saveToOriginal(lib);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_FALSE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mExistingSymbolFile.dir().exists());
    EXPECT_TRUE(mNewSymbolFile.exists());

    lib.addSymbol(*sym);
    saveToTemporary(lib);
    EXPECT_EQ(2, lib.getSymbols().count());
    EXPECT_FALSE(mExistingSymbolFile.exists());
    EXPECT_TRUE(mNewSymbolFile.exists());
  }
  EXPECT_FALSE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mExistingSymbolFile.dir().exists());
  EXPECT_TRUE(mNewSymbolFile.exists());
}

TEST_F(ProjectLibraryTest,
       testRemoveSymbol_SaveToTemporary_AddSymbolCopy_SaveToOriginal) {
  ElementName copyName("New Symbol Copy");
  {
    ProjectLibrary   lib(mLibDir, false, false);
    library::Symbol* sym = getFirstSymbol(lib);

    lib.removeSymbol(*sym);
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());

    // add new symbol with same UUID as the already added symbol
    QScopedPointer<library::Symbol> symCopy(new library::Symbol(
        sym->getUuid(), Version::fromString("1"), "", copyName, "", ""));
    lib.addSymbol(*symCopy.take());
    saveToTemporary(lib);
    saveToOriginal(lib);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(
        mExistingSymbolFile.exists());  // same path as the copied symbol
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());

  // check the name of the saved symbol to be sure the right symbol was saved
  library::Symbol symbol(FilePath(mExistingSymbolFile.dir().absolutePath()),
                         true);
  EXPECT_EQ(copyName, symbol.getNames().getDefaultValue());
}

TEST_F(ProjectLibraryTest, testSavingToExistingEmptyDirectory) {
  ProjectLibrary lib(mLibDir, false, false);

  // already create the destination directory to see if saving still works
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
  FileUtils::makePath(FilePath(mNewSymbolFile.dir().absolutePath()));
  EXPECT_TRUE(mNewSymbolFile.dir().exists());

  lib.addSymbol(*mNewSymbol.take());
  saveToTemporary(lib);
  saveToOriginal(lib);
  EXPECT_TRUE(mNewSymbolFile.exists());
}

TEST_F(ProjectLibraryTest, testIfExistingSymbolIsUpgradedOnlyOnce) {
  ProjectLibrary lib(mLibDir, false, false);
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
  saveToOriginal(lib);
  EXPECT_NE(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // upgraded!
  modifyExistingSymbol();
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
  saveToOriginal(lib);
  EXPECT_EQ(mExistingSymbolCreationSize,
            mExistingSymbolFile.size());  // not upgraded
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace project
}  // namespace librepcb
