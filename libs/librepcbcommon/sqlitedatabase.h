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

#ifndef LIBREPCB_SQLITEDATABASE_H
#define LIBREPCB_SQLITEDATABASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtSql>
#include "exceptions.h"
#include "fileio/filepath.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class SQLiteDatabase
 ****************************************************************************************/

/**
 * @brief The SQLiteDatabase class
 *
 * @author ubruhin
 * @date 2016-09-03
 */
class SQLiteDatabase final : public QObject
{
        Q_OBJECT

    public:

        // Types
        class TransactionScopeGuard final
        {
            public:
                TransactionScopeGuard() = delete;
                TransactionScopeGuard(const TransactionScopeGuard& other) = delete;
                TransactionScopeGuard(SQLiteDatabase& db) throw (Exception);
                ~TransactionScopeGuard() noexcept;
                void commit() throw (Exception);
                TransactionScopeGuard& operator=(const TransactionScopeGuard& rhs) = delete;
            private:
                SQLiteDatabase& mDb;
                bool mIsCommited;
        };


        // Constructors / Destructor
        SQLiteDatabase() = delete;
        SQLiteDatabase(const SQLiteDatabase& other) = delete;
        SQLiteDatabase(const FilePath& filepath) throw (Exception);
        ~SQLiteDatabase() noexcept;


        // SQL Commands
        void beginTransaction() throw (Exception);
        void commitTransaction() throw (Exception);
        void rollbackTransaction() throw (Exception);
        void clearTable(const QString& table) throw (Exception);


        // General Methods
        QSqlQuery prepareQuery(const QString& query) const throw (Exception);
        int insert(QSqlQuery& query) throw (Exception);
        void exec(QSqlQuery& query) throw (Exception);
        void exec(const QString& query) throw (Exception);


        // Operator Overloadings
        SQLiteDatabase& operator=(const SQLiteDatabase& rhs) = delete;


    private: // Data

        QSqlDatabase mDb;
        //int mNestedTransactionCount;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_SQLITEDATABASE_H
