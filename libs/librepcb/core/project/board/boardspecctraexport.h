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
#include "../../serialization/sexpression.h"
#include "../../types/length.h"

#include <QtCore>

#include <map>
#include <memory>
#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class BI_FootprintPad;
class BI_Via;
class Board;
class Footprint;
class Path;

/*******************************************************************************
 *  Class BoardSpecctraExport
 ******************************************************************************/

/**
 * @brief Specctra DSN Export
 */
class BoardSpecctraExport final : public QObject {
  Q_OBJECT

  struct ComponentData {
    std::unique_ptr<SExpression> image;
    std::vector<std::unique_ptr<SExpression>> placements;
  };

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
  std::unique_ptr<SExpression> genStructure() const;
  std::unique_ptr<SExpression> genPlacement(
      std::map<QString, ComponentData>& components) const;
  std::unique_ptr<SExpression> genLibrary(
      std::map<QString, ComponentData>& components,
      std::vector<std::unique_ptr<SExpression>>& padStacks) const;
  std::unique_ptr<SExpression> genNetwork() const;
  std::unique_ptr<SExpression> genWiring() const;

  std::unique_ptr<SExpression> toPlacement(const BI_Device& dev) const;
  std::unique_ptr<SExpression> toImage(
      const QString& id, const Footprint& footprint,
      std::vector<std::unique_ptr<SExpression>>& padStacks) const;
  std::unique_ptr<SExpression> toPath(const QString& layer,
                                      const UnsignedLength& width,
                                      const Path& path, bool multiline) const;
  std::unique_ptr<SExpression> toPadStack(const BI_FootprintPad& pad) const;
  std::unique_ptr<SExpression> toToken(const Length& length) const;

  // Private Member Variables
  const Board& mBoard;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
