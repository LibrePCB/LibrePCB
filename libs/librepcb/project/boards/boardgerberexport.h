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

#ifndef LIBREPCB_PROJECT_BOARDGERBEREXPORT_H
#define LIBREPCB_PROJECT_BOARDGERBEREXPORT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/units/all_length_units.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Polygon;
class Ellipse;
class GerberGenerator;

namespace project {

class Project;
class Board;
class BI_Via;
class BI_Footprint;
class BI_FootprintPad;

/*****************************************************************************************
 *  Class BoardGerberExport
 ****************************************************************************************/

/**
 * @brief The BoardGerberExport class
 *
 * @author ubruhin
 * @date 2016-01-10
 */
class BoardGerberExport final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BoardGerberExport() = delete;
        BoardGerberExport(const BoardGerberExport& other) = delete;
        BoardGerberExport(const Board& board, const FilePath& outputDir) noexcept;
        ~BoardGerberExport() noexcept;

        // General Methods
        void exportAllLayers() const;

        // Operator Overloadings
        BoardGerberExport& operator=(const BoardGerberExport& rhs) = delete;


    private:

        // Private Methods
        void exportDrillsPTH() const;
        void exportLayerBoardOutlines() const;
        void exportLayerTopCopper() const;
        void exportLayerTopSolderMask() const;
        void exportLayerTopSilkscreen() const;
        void exportLayerBottomCopper() const;
        void exportLayerBottomSolderMask() const;
        void exportLayerBottomSilkscreen() const;

        void drawLayer(GerberGenerator& gen, const QString& layerName) const;
        void drawVia(GerberGenerator& gen, const BI_Via& via, const QString& layerName) const;
        void drawFootprint(GerberGenerator& gen, const BI_Footprint& footprint, const QString& layerName) const;
        void drawFootprintPad(GerberGenerator& gen, const BI_FootprintPad& pad, const QString& layerName) const;

        FilePath getOutputFilePath(const QString& suffix) const noexcept;

        // Static Methods
        static Length calcWidthOfLayer(const Length& width, const QString& name) noexcept;


        // Private Member Variables
        const Project& mProject;
        const Board& mBoard;
        FilePath mOutputDirectory;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARDGERBEREXPORT_H
