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
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/projectlibrary.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ProjectLibraryTest : public ::testing::Test {
protected:
  FilePath mTempDir;
  FilePath mLibDir;
  std::shared_ptr<TransactionalFileSystem> mTempFs;
  std::shared_ptr<TransactionalFileSystem> mLibFs;
  std::unique_ptr<Symbol> mExistingSymbol;
  QFileInfo mExistingSymbolFile;
  std::unique_ptr<Symbol> mNewSymbol;
  QFileInfo mNewSymbolFile;
  qint64 mNewSymbolCreationSize;

  ProjectLibraryTest() {
    mTempDir = FilePath::getRandomTempPath();
    mLibDir = mTempDir.getPathTo("project library test");
    mTempFs = TransactionalFileSystem::openRW(mTempDir);
    mLibFs = TransactionalFileSystem::openRW(mLibDir);

    // create symbol inside project library
    mExistingSymbol.reset(new Symbol(Uuid::createRandom(),
                                     Version::fromString("1"), "",
                                     ElementName("Existing Symbol"), "", ""));
    TransactionalDirectory libSymDir(mLibFs, "sym");
    mExistingSymbol->saveIntoParentDirectory(libSymDir);
    mLibFs->save();
    mExistingSymbolFile.setFile(
        mLibDir
            .getPathTo(QString("sym/%1/symbol.lp")
                           .arg(mExistingSymbol->getUuid().toStr()))
            .toStr());

    // create symbol outside the project library (emulating workspace library)
    mNewSymbol.reset(new Symbol(Uuid::createRandom(), Version::fromString("1"),
                                "", ElementName("New Symbol"), "", ""));
    TransactionalDirectory tempSymDir(mTempFs);
    mNewSymbol->saveIntoParentDirectory(tempSymDir);
    mTempFs->save();
    mNewSymbolFile.setFile(
        mLibDir
            .getPathTo(
                QString("sym/%1/symbol.lp").arg(mNewSymbol->getUuid().toStr()))
            .toStr());

    // modify new symbol to detect the file format upgrade
    QByteArray newContent = mNewSymbol->getDirectory().read("symbol.lp") + " ";
    mNewSymbolCreationSize = newContent.size();
    mNewSymbol->getDirectory().write("symbol.lp", newContent);
    mTempFs->save();

    // disable caching to get correct results
    mExistingSymbolFile.setCaching(false);
    mNewSymbolFile.setCaching(false);
  }

  virtual ~ProjectLibraryTest() { QDir(mTempDir.toStr()).removeRecursively(); }

  Symbol* getFirstSymbol(ProjectLibrary& lib) {
    if (lib.getSymbols().isEmpty()) throw LogicError(__FILE__, __LINE__);
    return lib.getSymbols().values().first();
  }

  void saveToDisk() {
    mLibFs->save();
    mTempFs->save();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ProjectLibraryTest, testLoadSymbol) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mExistingSymbol.release());
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
}

TEST_F(ProjectLibraryTest, testAddSymbol) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mNewSymbol.release());
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.dir().exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
}

TEST_F(ProjectLibraryTest, testAddSymbol_Save) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mNewSymbol.release());
    saveToDisk();
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_TRUE(mNewSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_TRUE(mNewSymbolFile.exists());
  EXPECT_NE(mNewSymbolCreationSize,
            mNewSymbolFile.size());  // upgraded!
}

TEST_F(ProjectLibraryTest, testAddRemoveSymbol) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mNewSymbol);
    lib.removeSymbol(*mNewSymbol.release());
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.dir().exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
}

TEST_F(ProjectLibraryTest, testAddRemoveSymbol_Save) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mNewSymbol);
    lib.removeSymbol(*mNewSymbol.release());
    saveToDisk();
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.exists());
    EXPECT_FALSE(mNewSymbolFile.dir().exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.exists());
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
}

TEST_F(ProjectLibraryTest, testRemoveSymbol) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mExistingSymbol.release());
    lib.removeSymbol(*getFirstSymbol(lib));
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
}

TEST_F(ProjectLibraryTest, testRemoveSymbol_Save) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mExistingSymbol.release());
    lib.removeSymbol(*getFirstSymbol(lib));
    saveToDisk();
    EXPECT_EQ(0, lib.getSymbols().count());
    EXPECT_FALSE(mExistingSymbolFile.exists());
    EXPECT_FALSE(mExistingSymbolFile.dir().exists());
  }
  EXPECT_FALSE(mExistingSymbolFile.exists());
  EXPECT_FALSE(mExistingSymbolFile.dir().exists());
}

TEST_F(ProjectLibraryTest, testRemoveAddSymbol) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mExistingSymbol.release());
    Symbol* sym = getFirstSymbol(lib);
    lib.removeSymbol(*sym);
    lib.addSymbol(*sym);
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
}

TEST_F(ProjectLibraryTest, testRemoveAddSymbol_Save) {
  {
    ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(mLibFs)));
    lib.addSymbol(*mExistingSymbol.release());
    Symbol* sym = getFirstSymbol(lib);
    lib.removeSymbol(*sym);
    lib.addSymbol(*sym);
    saveToDisk();
    EXPECT_EQ(1, lib.getSymbols().count());
    EXPECT_TRUE(mExistingSymbolFile.exists());
  }
  EXPECT_TRUE(mExistingSymbolFile.exists());
}

TEST_F(ProjectLibraryTest, testSavingToExistingEmptyDirectory) {
  ProjectLibrary lib(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mLibFs)));

  // already create the destination directory to see if saving still works
  EXPECT_FALSE(mNewSymbolFile.dir().exists());
  FileUtils::makePath(FilePath(mNewSymbolFile.dir().absolutePath()));
  EXPECT_TRUE(mNewSymbolFile.dir().exists());

  lib.addSymbol(*mNewSymbol.release());
  saveToDisk();
  EXPECT_TRUE(mNewSymbolFile.exists());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
