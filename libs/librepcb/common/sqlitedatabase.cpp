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
#include "sqlitedatabase.h"

#include "uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class TransactionScopeGuard
 ******************************************************************************/

SQLiteDatabase::TransactionScopeGuard::TransactionScopeGuard(SQLiteDatabase& db)
  : mDb(db), mIsCommited(false) {
  mDb.beginTransaction();  // can throw
}

void SQLiteDatabase::TransactionScopeGuard::commit() {
  mDb.commitTransaction();  // can throw
  mIsCommited = true;
}

SQLiteDatabase::TransactionScopeGuard::~TransactionScopeGuard() noexcept {
  if (!mIsCommited) {
    try {
      mDb.rollbackTransaction();  // can throw
    } catch (Exception& e) {
      qCritical() << "Could not rollback database transaction:" << e.getMsg();
    }
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SQLiteDatabase::SQLiteDatabase(const FilePath& filepath)
  : QObject(nullptr)  //, mNestedTransactionCount(0)
{
  // create database (use random UUID as connection name)
  mDb = QSqlDatabase::addDatabase("QSQLITE", Uuid::createRandom().toStr());
  mDb.setDatabaseName(filepath.toStr());

  // check if database is valid
  if (!mDb.isValid()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Invalid database: \"%1\"")).arg(filepath.toNative()));
  }

  // open the database
  if (!mDb.open()) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString(tr("Could not open database: \"%1\""))
                           .arg(filepath.toNative()));
  }

  // set SQLite options
  exec("PRAGMA foreign_keys = ON");  // can throw
  enableSqliteWriteAheadLogging();   // can throw

  // check if all required features are available
  Q_ASSERT(mDb.driver() && mDb.driver()->hasFeature(QSqlDriver::Transactions));
  Q_ASSERT(mDb.driver() &&
           mDb.driver()->hasFeature(QSqlDriver::PreparedQueries));
  Q_ASSERT(mDb.driver() && mDb.driver()->hasFeature(QSqlDriver::LastInsertId));
  Q_ASSERT(getSqliteCompileOptions()["THREADSAFE"] == "1");  // can throw
}

SQLiteDatabase::~SQLiteDatabase() noexcept {
  mDb.close();
}

/*******************************************************************************
 *  SQL Commands
 ******************************************************************************/

void SQLiteDatabase::beginTransaction() {
  // Q_ASSERT(mNestedTransactionCount >= 0);
  // if (mNestedTransactionCount == 0) {
  if (!mDb.transaction()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not start database transaction."));
  }
  //}
  // mNestedTransactionCount++;
}

void SQLiteDatabase::commitTransaction() {
  // Q_ASSERT(mNestedTransactionCount >= 0);
  // if (mNestedTransactionCount == 1) {
  if (!mDb.commit()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not commit database transaction."));
  }
  //} else if (mNestedTransactionCount == 0) {
  //    throw RuntimeError(__FILE__, __LINE__,
  //        tr("Cannot commit database transaction because no one is active."));
  //}
  // mNestedTransactionCount--;
}

void SQLiteDatabase::rollbackTransaction() {
  // Q_ASSERT(mNestedTransactionCount >= 0);
  // if (mNestedTransactionCount == 1) {
  if (!mDb.rollback()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not rollback database transaction."));
  }
  //} else if (mNestedTransactionCount == 0) {
  //    throw RuntimeError(__FILE__, __LINE__,
  //        tr("Cannot rollback database transaction because no one is
  //        active."));
  //}
  // mNestedTransactionCount--;
}

void SQLiteDatabase::clearTable(const QString& table) {
  exec("DELETE FROM " % table);  // can throw
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QSqlQuery SQLiteDatabase::prepareQuery(const QString& query) const {
  QSqlQuery q(mDb);
  if (!q.prepare(query)) {
    qDebug() << q.lastError().databaseText();
    qDebug() << q.lastError().driverText();
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Error while preparing SQL query: %1")).arg(query));
  }
  return q;
}

int SQLiteDatabase::insert(QSqlQuery& query) {
  exec(query);  // can throw

  bool ok = false;
  int  id = query.lastInsertId().toInt(&ok);
  if (ok) {
    return id;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString(tr("Error while executing SQL query: %1"))
                           .arg(query.lastQuery()));
  }
}

void SQLiteDatabase::exec(QSqlQuery& query) {
  if (!query.exec()) {
    qDebug() << query.lastError().databaseText();
    qDebug() << query.lastError().driverText();
    throw RuntimeError(__FILE__, __LINE__,
                       QString(tr("Error while executing SQL query: %1"))
                           .arg(query.lastQuery()));
  }
}

void SQLiteDatabase::exec(const QString& query) {
  QSqlQuery q = prepareQuery(query);
  exec(q);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SQLiteDatabase::enableSqliteWriteAheadLogging() {
  QSqlQuery query("PRAGMA journal_mode=WAL", mDb);
  exec(query);  // can throw
  bool    success = query.first();
  QString result  = query.value(0).toString();
  if ((!success) || (result != "wal")) {
    throw LogicError(
        __FILE__, __LINE__,
        QString(tr("Could not enable SQLite Write-Ahead Logging: \"%1\""))
            .arg(result));
  }
}

QHash<QString, QString> SQLiteDatabase::getSqliteCompileOptions() {
  QHash<QString, QString> options;
  QSqlQuery               query("PRAGMA compile_options", mDb);
  exec(query);  // can throw
  while (query.next()) {
    QString option = query.value(0).toString();
    QString key    = option.section('=', 0, 0);
    QString value  = option.section('=', 1, -1);
    options.insert(key, value);
  }
  return options;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
