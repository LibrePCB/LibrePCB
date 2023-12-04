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

#ifndef LIBREPCB_CORE_SYMBOLPAINTER_H
#define LIBREPCB_CORE_SYMBOLPAINTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../export/graphicsexport.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circle;
class Polygon;
class Symbol;
class SymbolPin;
class Text;

/*******************************************************************************
 *  Class SymbolPainter
 ******************************************************************************/

/**
 * @brief Paints a ::librepcb::Symbol to a QPainter
 *
 * Used for ::librepcb::GraphicsExport.
 */
class SymbolPainter final : public GraphicsPagePainter {
public:
  // Constructors / Destructor
  SymbolPainter() = delete;
  explicit SymbolPainter(const Symbol& symbol) noexcept;
  SymbolPainter(const SymbolPainter& other) = delete;
  ~SymbolPainter() noexcept;

  // General Methods
  void paint(QPainter& painter,
             const GraphicsExportSettings& settings) const noexcept override;

  // Operator Overloadings
  SymbolPainter& operator=(const SymbolPainter& rhs) = delete;

private:  // Data
  QFont mDefaultFont;
  QList<SymbolPin> mPins;
  QList<Polygon> mPolygons;
  QList<Circle> mCircles;
  QList<Text> mTexts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
