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
#include <QtConcurrent>
#include <gtest/gtest.h>
#include <librepcb/common/sqlitedatabase.h>
#include <librepcb/common/fileio/fileutils.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class SQLiteDatabaseTest : public ::testing::Test
{
    protected:

        virtual void SetUp() override
        {
            // create temporary, empty directory
            mTempDir = FilePath::getApplicationTempPath().getPathTo("SQLiteDatabaseTest");
            mTempDbFilePath = mTempDir.getPathTo("db.sqlite");
            if (mTempDir.isExistingDir()) {
                FileUtils::removeDirRecursively(mTempDir); // can throw
            }
            FileUtils::makePath(mTempDir);
        }

        virtual void TearDown() override
        {
            // remove temporary directory
            FileUtils::removeDirRecursively(mTempDir); // can throw
        }

        enum ThreadOption {
            READING = (0<<0),
            WRITING = (1<<0),
            NO_TRANSACTION = (0<<1),
            TRANSACTION = (1<<1),
        };

        struct WorkerResult {
            qint64 rowCount;
            QString errorMsg;
        };

        static WorkerResult threadWorker(FilePath fp, int options, int duration) noexcept
        {
            try {
                qint64 count = 0;
                SQLiteDatabase db(fp);
                if (options & TRANSACTION) {
                    db.beginTransaction();
                }
                qint64 start = QDateTime::currentMSecsSinceEpoch();
                while (QDateTime::currentMSecsSinceEpoch() < start + duration) {
                    if (options & WRITING) {
                        db.exec("INSERT INTO test (name) VALUES ('hello')");
                    } else {
                        db.exec("SELECT id, name FROM test WHERE id = 1");
                    }
                    ++count;
                }
                if (options & TRANSACTION) {
                    db.commitTransaction();
                }
                return WorkerResult{count, QString()};
            } catch (const Exception& e) {
                return WorkerResult{-1, e.getUserMsg()};
            }
        }

        QFuture<WorkerResult> startWorkerThread(QThreadPool* pool, int options, int duration) noexcept
        {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)) // QtConcurrent::run(QThreadPool*, ...) requires Qt>=5.4
            auto future = QtConcurrent::run(pool, threadWorker, mTempDbFilePath, options, duration);
#else
            if (QThreadPool::globalInstance()->maxThreadCount() < pool->maxThreadCount()) {
                QThreadPool::globalInstance()->setMaxThreadCount(pool->maxThreadCount());
            }
            auto future = QtConcurrent::run(threadWorker, mTempDbFilePath, options, duration);
#endif
            mWorkerThreads.append(future);
            return future;
        }

        void waitUntilAllWorkersFinished() noexcept
        {
            while (!mWorkerThreads.isEmpty()) {
                mWorkerThreads.takeFirst().waitForFinished();
            }
        }

        FilePath mTempDir;
        FilePath mTempDbFilePath;
        QList<QFuture<WorkerResult>> mWorkerThreads;
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_F(SQLiteDatabaseTest, testIfContructorCreatesFile)
{
    EXPECT_FALSE(mTempDbFilePath.isExistingFile());
    { SQLiteDatabase db(mTempDbFilePath); } // object is created and deleted on this line!
    EXPECT_TRUE(mTempDbFilePath.isExistingFile());
}

TEST_F(SQLiteDatabaseTest, testExecQuery)
{
    SQLiteDatabase db(mTempDbFilePath);
    db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL)");
}

TEST_F(SQLiteDatabaseTest, testPreparedQuery)
{
    SQLiteDatabase db(mTempDbFilePath);
    db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
    QSqlQuery query = db.prepareQuery("INSERT INTO test (name) VALUES (:name)");
    query.bindValue(":name", "hello");
    db.exec(query);
}

TEST_F(SQLiteDatabaseTest, testInsert)
{
    SQLiteDatabase db(mTempDbFilePath);
    db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
    for (int i = 0; i < 100; ++i) {
        QSqlQuery query = db.prepareQuery("INSERT INTO test (name) VALUES (:name)");
        query.bindValue(":name", QString("row %1").arg(i));
        int id = db.insert(query);
        EXPECT_EQ(i + 1, id);
    }
}

TEST_F(SQLiteDatabaseTest, testClearExistingTable)
{
    SQLiteDatabase db(mTempDbFilePath);
    db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
    db.exec("INSERT INTO test (name) VALUES ('hello')");
    EXPECT_NO_THROW(db.clearTable("test"));
    EXPECT_NO_THROW(db.clearTable("test")); // clearing an empty table should also work
}

TEST_F(SQLiteDatabaseTest, testClearNonExistingTable)
{
    SQLiteDatabase db(mTempDbFilePath);
    EXPECT_THROW(db.clearTable("test"), Exception);
}

TEST_F(SQLiteDatabaseTest, testTransactionScopeGuardCommit)
{
    SQLiteDatabase db(mTempDbFilePath);
    {
        SQLiteDatabase::TransactionScopeGuard tsg(db);
        db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
        db.exec("INSERT INTO test (name) VALUES ('hello')");
        tsg.commit();
    }
    EXPECT_NO_THROW(db.clearTable("test"));
}

TEST_F(SQLiteDatabaseTest, testTransactionScopeGuardRollback)
{
    SQLiteDatabase db(mTempDbFilePath);
    {
        SQLiteDatabase::TransactionScopeGuard tsg(db);
        db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");
        db.exec("INSERT INTO test (name) VALUES ('hello')");
    }
    EXPECT_THROW(db.clearTable("test"), Exception);
}

TEST_F(SQLiteDatabaseTest, testMultipleInstancesInSameThread)
{
    SQLiteDatabase db1(mTempDbFilePath);
    SQLiteDatabase db2(mTempDbFilePath);
    db1.exec("CREATE TABLE test1 (`id` INTEGER PRIMARY KEY NOT NULL)");
    db2.exec("CREATE TABLE test2 (`id` INTEGER PRIMARY KEY NOT NULL)");
    EXPECT_NO_THROW(db1.clearTable("test2"));
    EXPECT_NO_THROW(db1.clearTable("test1"));
}

TEST_F(SQLiteDatabaseTest, testConcurrentAccessFromMultipleThreads)
{
    // create thread pool to ensure that every worker really runs in a separate thread
    QThreadPool pool;
    pool.setMaxThreadCount(8);

    // prepare database
    SQLiteDatabase db(mTempDbFilePath);
    db.exec("CREATE TABLE test (`id` INTEGER PRIMARY KEY NOT NULL, `name` TEXT)");

    // run worker threads (2 sequential writers and 4 parallel readers)
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    QFuture<WorkerResult> w1 = startWorkerThread(&pool, WRITING | TRANSACTION, 5000);
    QFuture<WorkerResult> r1 = startWorkerThread(&pool, READING | TRANSACTION, 10000);
    QFuture<WorkerResult> r2 = startWorkerThread(&pool, READING | TRANSACTION, 10000);
    QFuture<WorkerResult> r3 = startWorkerThread(&pool, READING | NO_TRANSACTION, 10000);
    QFuture<WorkerResult> r4 = startWorkerThread(&pool, READING | NO_TRANSACTION, 10000);
    w1.waitForFinished();
    QFuture<WorkerResult> w2 = startWorkerThread(&pool, WRITING | NO_TRANSACTION, 5000);
    waitUntilAllWorkersFinished();
    qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;

    // get row count
    QSqlQuery query = db.prepareQuery("SELECT COUNT(*) FROM test");
    db.exec(query);
    ASSERT_TRUE(query.first());
    qint64 rowCount = query.value(0).toLongLong();

    // validate results
    EXPECT_GT(w1.result().rowCount, 0) << qPrintable(w1.result().errorMsg);
    EXPECT_GT(w2.result().rowCount, 0) << qPrintable(w2.result().errorMsg);
    EXPECT_GT(r1.result().rowCount, 0) << qPrintable(r1.result().errorMsg);
    EXPECT_GT(r2.result().rowCount, 0) << qPrintable(r2.result().errorMsg);
    EXPECT_GT(r3.result().rowCount, 0) << qPrintable(r3.result().errorMsg);
    EXPECT_GT(r4.result().rowCount, 0) << qPrintable(r4.result().errorMsg);
    EXPECT_GT(rowCount, 0);
    EXPECT_EQ(rowCount, w1.result().rowCount + w2.result().rowCount);
    EXPECT_GE(duration, 10000);
    EXPECT_LE(duration, 14000);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
