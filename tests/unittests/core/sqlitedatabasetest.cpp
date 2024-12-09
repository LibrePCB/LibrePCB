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
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/sqlitedatabase.h>

#include <QtConcurrent>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SQLiteDatabaseTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    // create temporary, empty directory
    mTempDir = FilePath::getRandomTempPath();
    mTempDbFilePath = mTempDir.getPathTo("db.sqlite");
    if (mTempDir.isExistingDir()) {
      FileUtils::removeDirRecursively(mTempDir);  // can throw
    }
    FileUtils::makePath(mTempDir);
  }

  virtual void TearDown() override {
    // remove temporary directory
    FileUtils::removeDirRecursively(mTempDir);  // can throw
  }

  FilePath mTempDir;
  FilePath mTempDbFilePath;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(SQLiteDatabaseTest, testIfContructorCreatesFile) {
  EXPECT_FALSE(mTempDbFilePath.isExistingFile());
  {
    SQLiteDatabase db(mTempDbFilePath);
  }  // object is created and deleted on this line!
  EXPECT_TRUE(mTempDbFilePath.isExistingFile());
}

TEST_F(SQLiteDatabaseTest, testExecQuery) {
  SQLiteDatabase db(mTempDbFilePath);
  db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL)");
}

TEST_F(SQLiteDatabaseTest, testPreparedQuery) {
  SQLiteDatabase db(mTempDbFilePath);
  db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
  QSqlQuery query = db.prepareQuery("INSERT INTO test (name) VALUES (:name)");
  query.bindValue(":name", "hello");
  db.exec(query);
}

TEST_F(SQLiteDatabaseTest, testInsert) {
  SQLiteDatabase db(mTempDbFilePath);
  db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
  for (int i = 0; i < 100; ++i) {
    QSqlQuery query = db.prepareQuery("INSERT INTO test (name) VALUES (:name)");
    query.bindValue(":name", QString("row %1").arg(i));
    int id = db.insert(query);
    EXPECT_EQ(i + 1, id);
  }
}

TEST_F(SQLiteDatabaseTest, testClearExistingTable) {
  SQLiteDatabase db(mTempDbFilePath);
  db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
  db.exec("INSERT INTO test (name) VALUES ('hello')");
  EXPECT_NO_THROW(db.clearTable("test"));
  EXPECT_NO_THROW(
      db.clearTable("test"));  // clearing an empty table should also work
}

TEST_F(SQLiteDatabaseTest, testClearNonExistingTable) {
  SQLiteDatabase db(mTempDbFilePath);
  EXPECT_THROW(db.clearTable("test"), Exception);
}

TEST_F(SQLiteDatabaseTest, testTransactionScopeGuardCommit) {
  SQLiteDatabase db(mTempDbFilePath);
  {
    SQLiteDatabase::TransactionScopeGuard tsg(db);
    db.exec(
        "CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
    db.exec("INSERT INTO test (name) VALUES ('hello')");
    tsg.commit();
  }
  EXPECT_NO_THROW(db.clearTable("test"));
}

TEST_F(SQLiteDatabaseTest, testTransactionScopeGuardRollback) {
  SQLiteDatabase db(mTempDbFilePath);
  {
    SQLiteDatabase::TransactionScopeGuard tsg(db);
    db.exec(
        "CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
    db.exec("INSERT INTO test (name) VALUES ('hello')");
  }
  EXPECT_THROW(db.clearTable("test"), Exception);
}

TEST_F(SQLiteDatabaseTest, testMultipleInstancesInSameThread) {
  SQLiteDatabase db1(mTempDbFilePath);
  SQLiteDatabase db2(mTempDbFilePath);
  db1.exec("CREATE TABLE test1 (`id` INTEGER PRIMARY KEY NOT NULL)");
  db2.exec("CREATE TABLE test2 (`id` INTEGER PRIMARY KEY NOT NULL)");
  EXPECT_NO_THROW(db1.clearTable("test2"));
  EXPECT_NO_THROW(db1.clearTable("test1"));
}

TEST_F(SQLiteDatabaseTest, testConcurrentReadAccessWhileWriteTransaction) {
  // Prepare database.
  SQLiteDatabase db(mTempDbFilePath);
  db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");

  // Start worker thread which starts a transaction and writes to the database.
  FilePath fp = mTempDbFilePath;
  volatile int count = 0;
  volatile bool cancel = false;
  auto threadWorker = [fp, &count, &cancel]() {
    std::cout << "Worker thread started." << std::endl;
    SQLiteDatabase db(fp);
    db.beginTransaction();
    std::cout << "Transaction started." << std::endl;
    qint64 timeout = QDateTime::currentMSecsSinceEpoch() + 120000;
    while ((!cancel) && (QDateTime::currentMSecsSinceEpoch() < timeout)) {
      db.exec("INSERT INTO test (name) VALUES ('hello')");
      count = count + 1;
    }
    db.commitTransaction();
    std::cout << "Transaction committed." << std::endl;
  };
  auto future = QtConcurrent::run(threadWorker);

  // Wait until the thread has inserted the first values.
  qint64 timeout = QDateTime::currentMSecsSinceEpoch() + 120000;
  while ((count < 10) && (QDateTime::currentMSecsSinceEpoch() < timeout))
    ;
  ASSERT_GE(count, 10);
  std::cout << "Thread inserted " << count << " values." << std::endl;

  // Now we are sure the worker thread is continuously inserting new values.
  // So let's try to read from the database now.
  for (int i = 0; i < 10; ++i) {
    QSqlQuery query = db.prepareQuery("SELECT COUNT(*) FROM test");
    db.exec(query);
    ASSERT_TRUE(query.first());
    qint64 rowCount = query.value(0).toLongLong();
    std::cout << "Reading thread reads " << rowCount << " rows." << std::endl;
    EXPECT_EQ(rowCount, 0);  // Transaction not committed yet!
  }

  // Terminate the worker thread.
  cancel = true;
  future.waitForFinished();
  std::cout << "Worker thread exited." << std::endl;

  // Transaction finished -> row count should now be updated.
  QSqlQuery query = db.prepareQuery("SELECT COUNT(*) FROM test");
  db.exec(query);
  ASSERT_TRUE(query.first());
  qint64 rowCount = query.value(0).toLongLong();
  std::cout << "Reading thread reads " << rowCount << " rows." << std::endl;
  EXPECT_EQ(rowCount, count);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
