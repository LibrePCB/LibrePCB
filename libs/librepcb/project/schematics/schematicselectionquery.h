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

#ifndef LIBREPCB_PROJECT_SCHEMATICSELECTIONQUERY_H
#define LIBREPCB_PROJECT_SCHEMATICSELECTIONQUERY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class SI_Symbol;
class SI_SymbolPin;
class SI_NetSegment;
class SI_NetLine;
class SI_NetPoint;
class SI_NetLabel;

/*****************************************************************************************
 *  Class SchematicSelectionQuery
 ****************************************************************************************/

/**
 * @brief The SchematicSelectionQuery class
 */
class SchematicSelectionQuery final : public QObject
{
        Q_OBJECT

    public:

        // Types
        enum class NetPointFilter : uint32_t {
            Floating = (1 << 0),
            Attached = (1 << 1),
            AllConnectedLinesSelected = (1 << 2),
            All = (NetPointFilter::Floating | NetPointFilter::Attached)
        };
        Q_DECLARE_FLAGS(NetPointFilters, NetPointFilter)

        enum class NetLineFilter : uint32_t {
            Floating = (1 << 0),
            Attached = (1 << 1),
            All = (NetLineFilter::Floating | NetLineFilter::Attached)
        };
        Q_DECLARE_FLAGS(NetLineFilters, NetLineFilter)


        // Constructors / Destructor
        SchematicSelectionQuery() = delete;
        SchematicSelectionQuery(const SchematicSelectionQuery& other) = delete;
        SchematicSelectionQuery(const QList<SI_Symbol*>& symbols,
                                const QList<SI_NetSegment*>& netsegments,
                                QObject* parent = nullptr);
        ~SchematicSelectionQuery() noexcept;

        // Getters
        const QSet<SI_Symbol*>& getSymbols() const noexcept { return mResultSymbols; }
        const QSet<SI_NetPoint*>& getNetPoints() const noexcept { return mResultNetPoints; }
        const QSet<SI_NetLine*>& getNetLines() const noexcept { return mResultNetLines; }
        const QSet<SI_NetLabel*>& getNetLabels() const noexcept { return mResultNetLabels; }
        int getResultCount() const noexcept;
        bool isResultEmpty() const noexcept { return (getResultCount() == 0); }

        // General Methods
        void addSelectedSymbols() noexcept;
        void addSelectedNetPoints(NetPointFilters f) noexcept;
        void addSelectedNetLines(NetLineFilters f) noexcept;
        void addSelectedNetLabels() noexcept;
        void addNetPointsOfNetLines(NetLineFilters lf, NetPointFilters pf) noexcept;

        // Operator Overloadings
        SchematicSelectionQuery& operator=(const SchematicSelectionQuery& rhs) = delete;


    private:

        static bool doesNetPointMatchFilter(const SI_NetPoint& p, NetPointFilters f) noexcept;
        static bool doesNetLineMatchFilter(const SI_NetLine& l, NetLineFilters f) noexcept;

        // references to the Schematic object
        const QList<SI_Symbol*>& mSymbols;
        const QList<SI_NetSegment*>& mNetSegments;

        // query result
        QSet<SI_Symbol*> mResultSymbols;
        QSet<SI_NetPoint*> mResultNetPoints;
        QSet<SI_NetLine*> mResultNetLines;
        QSet<SI_NetLabel*> mResultNetLabels;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SchematicSelectionQuery::NetPointFilters)
Q_DECLARE_OPERATORS_FOR_FLAGS(SchematicSelectionQuery::NetLineFilters)

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SCHEMATICSELECTIONQUERY_H
