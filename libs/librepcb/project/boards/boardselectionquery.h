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

#ifndef LIBREPCB_PROJECT_BOARDSELECTIONQUERY_H
#define LIBREPCB_PROJECT_BOARDSELECTIONQUERY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/uuid.h>
#include <librepcb/common/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class BI_Device;
class BI_Footprint;
class BI_FootprintPad;
class BI_Via;
class BI_NetSegment;
class BI_NetLine;
class BI_NetPoint;
class BI_Polygon;

/*****************************************************************************************
 *  Class BoardSelectionQuery
 ****************************************************************************************/

/**
 * @brief The BoardSelectionQuery class
 */
class BoardSelectionQuery final : public QObject
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
        BoardSelectionQuery() = delete;
        BoardSelectionQuery(const BoardSelectionQuery& other) = delete;
        BoardSelectionQuery(const QMap<Uuid, BI_Device*>& deviceInstances,
                            const QList<BI_NetSegment*>& netsegments,
                            const QList<BI_Polygon*>& polygons,
                            QObject* parent = nullptr);
        ~BoardSelectionQuery() noexcept;

        // Getters
        //const QSet<BI_Device*>& getDeviceInstances() const noexcept { return mResultDeviceInstances; }
        const QSet<BI_Footprint*>& getFootprints() const noexcept { return mResultFootprints; }
        const QSet<BI_NetPoint*>& getNetPoints() const noexcept { return mResultNetPoints; }
        const QSet<BI_NetLine*>& getNetLines() const noexcept { return mResultNetLines; }
        const QSet<BI_Via*>& getVias() const noexcept { return mResultVias; }
        //const QSet<BI_Polygon*>& getPolygons() const noexcept { return mResultPolygons; }
        int getResultCount() const noexcept;
        bool isResultEmpty() const noexcept { return (getResultCount() == 0); }

        // General Methods
        void addSelectedFootprints() noexcept;
        void addSelectedVias() noexcept;
        void addSelectedNetPoints(NetPointFilters f) noexcept;
        void addSelectedNetLines(NetLineFilters f) noexcept;
        void addNetPointsOfNetLines(NetLineFilters lf, NetPointFilters pf) noexcept;

        // Operator Overloadings
        BoardSelectionQuery& operator=(const BoardSelectionQuery& rhs) = delete;


    private:

        static bool doesNetPointMatchFilter(const BI_NetPoint& p, NetPointFilters f) noexcept;
        static bool doesNetLineMatchFilter(const BI_NetLine& l, NetLineFilters f) noexcept;

        // references to the Board object
        const QMap<Uuid, BI_Device*>& mDevices;
        const QList<BI_NetSegment*>& mNetSegments;
        const QList<BI_Polygon*>& mPolygons;

        // query result
        //QSet<BI_Device*> mResultDeviceInstances;
        QSet<BI_Footprint*> mResultFootprints;
        QSet<BI_NetPoint*> mResultNetPoints;
        QSet<BI_NetLine*> mResultNetLines;
        QSet<BI_Via*> mResultVias;
        //QSet<BI_Polygon*> mResultPolygons;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(BoardSelectionQuery::NetPointFilters)
Q_DECLARE_OPERATORS_FOR_FLAGS(BoardSelectionQuery::NetLineFilters)

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARDSELECTIONQUERY_H
