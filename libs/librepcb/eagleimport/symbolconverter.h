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

#ifndef LIBREPCB_EAGLEIMPORT_SYMBOLCONVERTER_H
#define LIBREPCB_EAGLEIMPORT_SYMBOLCONVERTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/graphics/graphicslayername.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/

namespace parseagle {
class Symbol;
}

namespace librepcb {

namespace library {
class Symbol;
}

namespace eagleimport {

class ConverterDb;

/*******************************************************************************
 *  Class SymbolConverter
 ******************************************************************************/

/**
 * @brief The SymbolConverter class
 */
class SymbolConverter final {
public:
  // Constructors / Destructor
  SymbolConverter()                             = delete;
  SymbolConverter(const SymbolConverter& other) = delete;
  SymbolConverter(const parseagle::Symbol& symbol, ConverterDb& db) noexcept;
  ~SymbolConverter() noexcept;

  // General Methods
  std::unique_ptr<library::Symbol> generate() const;

  // Operator Overloadings
  SymbolConverter& operator=(const SymbolConverter& rhs) = delete;

private:
  QString                  createDescription() const noexcept;
  static GraphicsLayerName convertSchematicLayer(int eagleLayerId);

  const parseagle::Symbol& mSymbol;
  ConverterDb&             mDb;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif  // LIBREPCB_EAGLEIMPORT_SYMBOLCONVERTER_H
