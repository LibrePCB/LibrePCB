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

#ifndef LIBREPCB_CORE_BOARDGERBEREXPORT_H
#define LIBREPCB_CORE_BOARDGERBEREXPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../attribute/attributeprovider.h"
#include "../../fileio/filepath.h"
#include "../../types/length.h"

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Footprint;
class BI_FootprintPad;
class BI_Via;
class Board;
class BoardFabricationOutputSettings;
class Circle;
class ExcellonGenerator;
class GerberGenerator;
class Polygon;
class Project;

/*******************************************************************************
 *  Class BoardGerberExport
 ******************************************************************************/

/**
 * @brief The BoardGerberExport class
 */
class BoardGerberExport final : public QObject, public AttributeProvider {
  Q_OBJECT

public:
  enum class BoardSide { Top, Bottom };

  // Constructors / Destructor
  BoardGerberExport() = delete;
  BoardGerberExport(const BoardGerberExport& other) = delete;
  explicit BoardGerberExport(const Board& board) noexcept;
  ~BoardGerberExport() noexcept;

  // Getters
  FilePath getOutputDirectory(
      const BoardFabricationOutputSettings& settings) const noexcept;
  const QVector<FilePath>& getWrittenFiles() const noexcept {
    return mWrittenFiles;
  }

  // General Methods
  void exportPcbLayers(const BoardFabricationOutputSettings& settings) const;
  void exportComponentLayer(BoardSide side, const FilePath& filePath) const;

  // Inherited from AttributeProvider
  /// @copydoc ::librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc ::librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Operator Overloadings
  BoardGerberExport& operator=(const BoardGerberExport& rhs) = delete;

signals:
  void attributesChanged() override;

private:
  // Private Methods
  void exportDrills(const BoardFabricationOutputSettings& settings) const;
  void exportDrillsNpth(const BoardFabricationOutputSettings& settings) const;
  void exportDrillsPth(const BoardFabricationOutputSettings& settings) const;
  void exportLayerBoardOutlines(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerTopCopper(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerInnerCopper(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerBottomCopper(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerTopSolderMask(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerBottomSolderMask(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerTopSilkscreen(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerBottomSilkscreen(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerTopSolderPaste(
      const BoardFabricationOutputSettings& settings) const;
  void exportLayerBottomSolderPaste(
      const BoardFabricationOutputSettings& settings) const;

  int drawNpthDrills(ExcellonGenerator& gen) const;
  int drawPthDrills(ExcellonGenerator& gen) const;
  void drawLayer(GerberGenerator& gen, const QString& layerName) const;
  void drawVia(GerberGenerator& gen, const BI_Via& via,
               const QString& layerName, const QString& netName) const;
  void drawFootprint(GerberGenerator& gen, const BI_Footprint& footprint,
                     const QString& layerName) const;
  void drawFootprintPad(GerberGenerator& gen, const BI_FootprintPad& pad,
                        const QString& layerName) const;

  FilePath getOutputFilePath(QString path) const noexcept;

  // Static Methods
  static UnsignedLength calcWidthOfLayer(const UnsignedLength& width,
                                         const QString& name) noexcept;
  template <typename T>
  static QList<T*> sortedByUuid(const QList<T*>& list) noexcept {
    // sort a list of objects by their UUID to get reproducible gerber files
    QList<T*> copy = list;
    std::sort(copy.begin(), copy.end(), [](const T* o1, const T* o2) {
      return o1->getUuid() < o2->getUuid();
    });
    return copy;
  }

  // Private Member Variables
  const Project& mProject;
  const Board& mBoard;
  QDateTime mCreationDateTime;
  QString mProjectName;
  mutable int mCurrentInnerCopperLayer;
  mutable QVector<FilePath> mWrittenFiles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
