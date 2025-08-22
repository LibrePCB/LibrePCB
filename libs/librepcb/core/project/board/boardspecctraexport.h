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

#ifndef LIBREPCB_CORE_BOARDSPECCTRAEXPORT_H
#define LIBREPCB_CORE_BOARDSPECCTRAEXPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/point.h"

#include <QtCore>

#include <map>
#include <memory>
#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class BI_NetSegment;
class BI_Pad;
class BI_Via;
class Board;
class Hole;
class Layer;
class Path;
class Point;
class SExpression;

/*******************************************************************************
 *  Class BoardSpecctraExport
 ******************************************************************************/

/**
 * @brief Specctra DSN Export
 */
class BoardSpecctraExport final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardSpecctraExport() = delete;
  BoardSpecctraExport(const BoardSpecctraExport& other) = delete;
  explicit BoardSpecctraExport(const Board& board) noexcept;
  ~BoardSpecctraExport() noexcept;

  // General Methods
  QByteArray generate() const;

  // Operator Overloadings
  BoardSpecctraExport& operator=(const BoardSpecctraExport& rhs) = delete;

private:
  std::unique_ptr<SExpression> genParser() const;
  std::unique_ptr<SExpression> genResolution() const;
  std::unique_ptr<SExpression> genStructure(
      std::vector<std::unique_ptr<SExpression>>& viaPadStacks) const;
  std::unique_ptr<SExpression> genStructureRule() const;
  std::unique_ptr<SExpression> genPlacement() const;
  std::unique_ptr<SExpression> genLibrary(
      std::vector<std::unique_ptr<SExpression>>& fptPadStacks,
      std::vector<std::unique_ptr<SExpression>>& viaPadStacks) const;
  std::unique_ptr<SExpression> genLibraryImage(
      const BI_Device& dev,
      std::vector<std::unique_ptr<SExpression>>& fptPadStacks) const;
  std::unique_ptr<SExpression> genLibraryPadStack(const BI_Pad& pad) const;
  std::unique_ptr<SExpression> genNetwork() const;
  std::unique_ptr<SExpression> genWiring() const;
  std::unique_ptr<SExpression> genWiringPadStack(const BI_Via& via) const;
  QString getWiringPadStackId(const BI_Via& via) const;

  template <typename THole>
  std::unique_ptr<SExpression> toKeepout(const QString& id,
                                         const THole& hole) const;
  std::unique_ptr<SExpression> toKeepout(
      const QString& id, const Path& path,
      const QSet<const Layer*>& layers) const;
  std::unique_ptr<SExpression> toPolygon(const QString& layer,
                                         const UnsignedLength& width,
                                         const Path& path,
                                         bool multiline) const;
  std::unique_ptr<SExpression> toPath(const QString& layer,
                                      const UnsignedLength& width,
                                      const Path& path, bool multiline) const;
  std::unique_ptr<SExpression> toCircle(const QString& layer,
                                        const PositiveLength& diameter,
                                        const Point& pos = Point()) const;
  std::unique_ptr<SExpression> toToken(const Length& length) const;
  static QString getNetName(const BI_NetSegment& ns) noexcept;
  static std::size_t addToPadStacks(
      std::vector<std::unique_ptr<SExpression>>& padStacks,
      std::unique_ptr<SExpression> padStack);

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs.
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

  // Private Member Variables
  const Board& mBoard;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
