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

#ifndef LIBREPCB_SQLITEDATABASE_H
#define LIBREPCB_SQLITEDATABASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "exceptions.h"
#include "fileio/filepath.h"

#include <QtCore>
#include <QtSql>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SQLiteDatabase
 ******************************************************************************/

/**
 * @brief The SQLiteDatabase class
 */
class SQLiteDatabase final : public QObject {
  Q_OBJECT

public:
  // Types
  class TransactionScopeGuard final {
  public:
    TransactionScopeGuard()                                   = delete;
    TransactionScopeGuard(const TransactionScopeGuard& other) = delete;
    TransactionScopeGuard(SQLiteDatabase& db);
    ~TransactionScopeGuard() noexcept;
    void                   commit();
    TransactionScopeGuard& operator=(const TransactionScopeGuard& rhs) = delete;

  private:
    SQLiteDatabase& mDb;
    bool            mIsCommited;
  };

  // Constructors / Destructor
  SQLiteDatabase()                            = delete;
  SQLiteDatabase(const SQLiteDatabase& other) = delete;
  SQLiteDatabase(const FilePath& filepath);
  ~SQLiteDatabase() noexcept;

  // SQL Commands
  void beginTransaction();
  void commitTransaction();
  void rollbackTransaction();
  void clearTable(const QString& table);

  // General Methods
  QSqlQuery prepareQuery(const QString& query) const;
  int       count(QSqlQuery& query);
  int       insert(QSqlQuery& query);
  void      exec(QSqlQuery& query);
  void      exec(const QString& query);

  // Operator Overloadings
  SQLiteDatabase& operator=(const SQLiteDatabase& rhs) = delete;

private:  // Methods
  /**
   * @brief Enable the "Write-Ahead Logging" (WAL) featur of SQLite
   *
   * @note LibrePCB requires to enable WAL to avoid blocking readers by writers.
   * If not enabled, the library scanner would also block all read-only accesses
   *       to the library database.
   *
   * @see http://www.sqlite.org/wal.html
   */
  void enableSqliteWriteAheadLogging();

  /**
   * @brief Get compile options of the SQLite driver library
   *
   * @return A hashmap of all compile options (without the "SQLITE_" prefix)
   *
   * @see https://sqlite.org/pragma.html#pragma_compile_options
   */
  QHash<QString, QString> getSqliteCompileOptions();

private:  // Data
  QSqlDatabase mDb;
  // int mNestedTransactionCount;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SQLITEDATABASE_H
