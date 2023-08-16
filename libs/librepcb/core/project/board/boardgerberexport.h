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
#include "../../export/excellongenerator.h"
#include "../../export/gerbergenerator.h"
#include "../../fileio/filepath.h"
#include "../../types/length.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

#include <functional>
#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class BI_FootprintPad;
class BI_Via;
class Board;
class BoardFabricationOutputSettings;
class Circle;
class GerberGenerator;
class Layer;
class Polygon;
class Project;

/*******************************************************************************
 *  Class BoardGerberExport
 ******************************************************************************/

/**
 * @brief The BoardGerberExport class
 */
class BoardGerberExport final : public QObject {
  Q_OBJECT

public:
  enum class BoardSide { Top, Bottom };
  typedef std::pair<const Layer*, const Layer*> LayerPair;
  typedef std::function<void(const FilePath&)> BeforeWriteCallback;

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

  // Setters
  void setRemoveObsoleteFiles(bool remove);
  void setBeforeWriteCallback(BeforeWriteCallback cb);

  // General Methods
  void exportPcbLayers(const BoardFabricationOutputSettings& settings) const;
  void exportComponentLayer(BoardSide side, const Uuid& assemblyVariant,
                            const FilePath& filePath) const;

  // Operator Overloadings
  BoardGerberExport& operator=(const BoardGerberExport& rhs) = delete;

private:
  // Private Methods
  void exportDrillsMerged(const BoardFabricationOutputSettings& settings) const;
  void exportDrillsNpth(const BoardFabricationOutputSettings& settings) const;
  void exportDrillsPth(const BoardFabricationOutputSettings& settings) const;
  void exportDrillsBlindBuried(
      const BoardFabricationOutputSettings& settings) const;
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
  QMap<LayerPair, QList<const BI_Via*> > getBlindBuriedVias() const;
  void drawLayer(GerberGenerator& gen, const Layer& layer) const;
  void drawVia(GerberGenerator& gen, const BI_Via& via, const Layer& layer,
               const QString& netName) const;
  void drawDevice(GerberGenerator& gen, const BI_Device& device,
                  const Layer& layer) const;
  void drawFootprintPad(GerberGenerator& gen, const BI_FootprintPad& pad,
                        const Layer& layer) const;
  void drawPolygon(GerberGenerator& gen, const Layer& layer,
                   const Path& outline, const UnsignedLength& lineWidth,
                   bool fill, GerberGenerator::Function function,
                   const tl::optional<QString>& net,
                   const QString& component) const;
  QVector<Path> getComponentOutlines(const BI_Device& device,
                                     const Layer& layer) const;

  std::unique_ptr<ExcellonGenerator> createExcellonGenerator(
      const BoardFabricationOutputSettings& settings,
      ExcellonGenerator::Plating plating) const;
  FilePath getOutputFilePath(QString path) const noexcept;
  QString getAttributeValue(const QString& key) const noexcept;
  void trackFileBeforeWrite(const FilePath& fp) const;

  // Static Methods
  static UnsignedLength calcWidthOfLayer(const UnsignedLength& width,
                                         const Layer& layer) noexcept;

  // Private Member Variables
  const Project& mProject;
  const Board& mBoard;
  bool mRemoveObsoleteFiles;
  BeforeWriteCallback mBeforeWriteCallback;
  QDateTime mCreationDateTime;
  QString mProjectName;
  mutable int mCurrentInnerCopperLayer;
  mutable const Layer* mCurrentStartLayer;
  mutable const Layer* mCurrentEndLayer;
  mutable QVector<FilePath> mWrittenFiles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
