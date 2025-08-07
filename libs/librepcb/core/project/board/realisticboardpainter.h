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

#ifndef LIBREPCB_CORE_REALISTICBOARDPAINTER_H
#define LIBREPCB_CORE_REALISTICBOARDPAINTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../export/graphicsexport.h"

#include <polyclipping/clipper.hpp>

#include <QtCore>
#include <QtGui>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SceneData3D;

/*******************************************************************************
 *  Class RealisticBoardPainter
 ******************************************************************************/

/**
 * @brief Paints a ::librepcb::Board in realistic mode to a QPainter
 *
 * Similar to ::librepcb::BoardPainter, but rendering physical layers instead
 * of logical layers, i.e. the output will be a realistic PCB with gray
 * body, golden copper, (typically) green solder resist and (typically)
 * white silkscreen instead of just rendering all objects the same way as
 * on screen.
 *
 * Used in conjuction with ::librepcb::GraphicsExport. Colors are obtained from
 * ::librepcb::SceneData3D and whether the top or the bottom side is rendered
 * is controlled by ::librepcb::GraphicsExportSettings::getMirror().
 *
 * @see ::librepcb::GraphicsPagePainter
 * @see ::librepcb::GraphicsExport
 * @see ::librepcb::SceneData3D
 */
class RealisticBoardPainter final : public GraphicsPagePainter {
public:
  // Constructors / Destructor
  RealisticBoardPainter() = delete;
  explicit RealisticBoardPainter(std::shared_ptr<SceneData3D> data);
  RealisticBoardPainter(const RealisticBoardPainter& other) = delete;
  ~RealisticBoardPainter() noexcept;

  // General Methods
  void paint(QPainter& painter,
             const GraphicsExportSettings& settings) const noexcept override;

  // Operator Overloadings
  RealisticBoardPainter& operator=(const RealisticBoardPainter& rhs) = delete;

private:  // Methods
  QVector<std::pair<QColor, QPainterPath>> getContent(
      const GraphicsExportSettings& settings) const noexcept;

private:  // Data
  const PositiveLength mMaxArcTolerance;
  std::shared_ptr<SceneData3D> mData;

  mutable QMutex mMutex;
  mutable bool mDataPreprocessed;
  mutable std::optional<ClipperLib::Paths> mCachedHoles;
  mutable std::optional<ClipperLib::Paths> mCachedCopperHoles;
  mutable std::optional<ClipperLib::Paths> mCachedBoardOutlines;
  mutable std::optional<ClipperLib::Paths> mCachedBoardArea;
  mutable std::optional<ClipperLib::Paths> mCachedSolderResistTop;
  mutable std::optional<ClipperLib::Paths> mCachedSolderResistBot;
  mutable QHash<QString, QPainterPath> mCachedContentPerLayer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
