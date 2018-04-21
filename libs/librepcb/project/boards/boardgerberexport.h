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
#include <librepcb/common/attributes/attributeprovider.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/units/all_length_units.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Polygon;
class Ellipse;
class ExcellonGenerator;
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
class BoardGerberExport final : public QObject, public AttributeProvider
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BoardGerberExport() = delete;
        BoardGerberExport(const BoardGerberExport& other) = delete;
        BoardGerberExport(const Board& board) noexcept;
        ~BoardGerberExport() noexcept;

        // Getters
        FilePath getOutputDirectory() const noexcept;

        // General Methods
        void exportAllLayers() const;

        // Inherited from AttributeProvider
        /// @copydoc librepcb::AttributeProvider::getBuiltInAttributeValue()
        QString getBuiltInAttributeValue(const QString& key) const noexcept override;
        /// @copydoc librepcb::AttributeProvider::getAttributeProviderParents()
        QVector<const AttributeProvider*> getAttributeProviderParents() const noexcept override;

        // Operator Overloadings
        BoardGerberExport& operator=(const BoardGerberExport& rhs) = delete;


    signals:
        void attributesChanged() override;


    private:

        // Private Methods
        void exportDrills() const;
        void exportDrillsNpth() const;
        void exportDrillsPth() const;
        void exportLayerBoardOutlines() const;
        void exportLayerTopCopper() const;
        void exportLayerInnerCopper() const;
        void exportLayerBottomCopper() const;
        void exportLayerTopSolderMask() const;
        void exportLayerBottomSolderMask() const;
        void exportLayerTopSilkscreen() const;
        void exportLayerBottomSilkscreen() const;
        void exportLayerTopSolderPaste() const;
        void exportLayerBottomSolderPaste() const;

        int drawNpthDrills(ExcellonGenerator& gen) const;
        int drawPthDrills(ExcellonGenerator& gen) const;
        void drawLayer(GerberGenerator& gen, const QString& layerName) const;
        void drawVia(GerberGenerator& gen, const BI_Via& via, const QString& layerName) const;
        void drawFootprint(GerberGenerator& gen, const BI_Footprint& footprint, const QString& layerName) const;
        void drawFootprintPad(GerberGenerator& gen, const BI_FootprintPad& pad, const QString& layerName) const;

        FilePath getOutputFilePath(const QString& suffix) const noexcept;

        // Static Methods
        static Length calcWidthOfLayer(const Length& width, const QString& name) noexcept;
        template <typename T>
        static QList<T*> sortedByUuid(const QList<T*>& list) noexcept {
            // sort a list of objects by their UUID to get reproducable gerber files
            QList<T*> copy = list;
            qSort(copy.begin(), copy.end(),
                  [](const T* o1, const T* o2){return o1->getUuid() < o2->getUuid();});
            return copy;
        }


        // Private Member Variables
        const Project& mProject;
        const Board& mBoard;
        mutable int mCurrentInnerCopperLayer;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARDGERBEREXPORT_H
