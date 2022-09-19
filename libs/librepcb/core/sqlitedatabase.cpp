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
#include "sqlitedatabase.h"

#include "exceptions.h"
#include "types/uuid.h"

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
      qCritical() << "Failed to roll back database transaction:" << e.getMsg();
    }
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SQLiteDatabase::SQLiteDatabase(const FilePath& filepath, QObject* parent)
  : QObject(parent) {
  // create database (use random UUID as connection name)
  mDb = QSqlDatabase::addDatabase("QSQLITE", Uuid::createRandom().toStr());
  mDb.setDatabaseName(filepath.toStr());

  // check if database is valid
  if (!mDb.isValid()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Invalid database: \"%1\"").arg(filepath.toNative()));
  }

  // open the database
  if (!mDb.open()) {
    qCritical() << "SQLiteDatabase error:" << mDb.lastError().text();
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not open database: \"%1\"").arg(filepath.toNative()));
  }

  // set SQLite options
  exec("PRAGMA foreign_keys = ON");  // can throw
  enableSqliteWriteAheadLogging();  // can throw

  // check if all required features are available
  Q_ASSERT(mDb.driver() && mDb.driver()->hasFeature(QSqlDriver::Transactions));
  Q_ASSERT(mDb.driver() &&
           mDb.driver()->hasFeature(QSqlDriver::PreparedQueries));
  Q_ASSERT(mDb.driver() && mDb.driver()->hasFeature(QSqlDriver::LastInsertId));
  Q_ASSERT(getSqliteCompileOptions().value("THREADSAFE") == "1");  // can throw
}

SQLiteDatabase::~SQLiteDatabase() noexcept {
  mDb.close();
}

/*******************************************************************************
 *  SQL Commands
 ******************************************************************************/

void SQLiteDatabase::beginTransaction() {
  if (!mDb.transaction()) {
    qCritical() << "SQLiteDatabase error:" << mDb.lastError().text();
    throw RuntimeError(
        __FILE__, __LINE__,
        "Could not start database transaction: " % mDb.lastError().text());
  }
}

void SQLiteDatabase::commitTransaction() {
  if (!mDb.commit()) {
    qCritical() << "SQLiteDatabase error:" << mDb.lastError().text();
    throw RuntimeError(
        __FILE__, __LINE__,
        "Could not commit database transaction: " % mDb.lastError().text());
  }
}

void SQLiteDatabase::rollbackTransaction() {
  if (!mDb.rollback()) {
    qCritical() << "SQLiteDatabase error:" << mDb.lastError().text();
    throw RuntimeError(
        __FILE__, __LINE__,
        "Could not rollback database transaction: " % mDb.lastError().text());
  }
}

void SQLiteDatabase::clearTable(const QString& table) {
  exec("DELETE FROM " % table);  // can throw
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QSqlQuery SQLiteDatabase::prepareQuery(QString query,
                                       const Replacements& replacements) const {
  for (auto it = replacements.begin(); it != replacements.end(); it++) {
    query.replace(it->first, it->second);
  }

  QSqlQuery q(mDb);
  if (!q.prepare(query)) {
    qCritical() << "SQLiteDatabase query:" << query;
    qCritical() << "SQLiteDatabase error:" << q.lastError().text();
    throw RuntimeError(__FILE__, __LINE__,
                       "Error while preparing SQL query: " % query % "\n" %
                           q.lastError().text());
  }
  return q;
}

int SQLiteDatabase::count(QSqlQuery& query) {
  exec(query);  // can throw

  int count = 0;
  bool success = query.next() && query.value(0).isValid();
  if (success) {
    count = query.value(0).toInt(&success);
  }
  if (success) {
    return count;
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

int SQLiteDatabase::insert(QSqlQuery& query) {
  exec(query);  // can throw

  bool ok = false;
  int id = query.lastInsertId().toInt(&ok);
  if (ok) {
    return id;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       "Error while executing SQL query: " % query.lastQuery());
  }
}

void SQLiteDatabase::exec(QSqlQuery& query) {
  if (!query.exec()) {
    qCritical() << "SQLiteDatabase query:" << query.lastQuery();
    qCritical() << "SQLiteDatabase error:" << query.lastError().text();
    throw RuntimeError(__FILE__, __LINE__,
                       "Error while executing SQL query: " % query.lastQuery() %
                           "\n" % query.lastError().text());
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
  bool success = query.first();
  QString result = query.value(0).toString();
  if ((!success) || (result != "wal")) {
    throw RuntimeError(
        __FILE__, __LINE__,
        "Could not enable SQLite Write-Ahead Logging: " % result);
  }
}

QHash<QString, QString> SQLiteDatabase::getSqliteCompileOptions() {
  QHash<QString, QString> options;
  QSqlQuery query("PRAGMA compile_options", mDb);
  exec(query);  // can throw
  while (query.next()) {
    QString option = query.value(0).toString();
    QString key = option.section('=', 0, 0);
    QString value = option.section('=', 1, -1);
    options.insert(key, value);
  }
  return options;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
