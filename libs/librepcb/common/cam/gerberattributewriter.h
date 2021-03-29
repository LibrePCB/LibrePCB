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

#ifndef LIBREPCB_GERBERATTRIBUTEWRITER_H
#define LIBREPCB_GERBERATTRIBUTEWRITER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GerberAttribute;

/*******************************************************************************
 *  Class GerberAttributeWriter
 ******************************************************************************/

/**
 * @brief A helper class to generate Gerber X2 attributes
 *
 * This class works with a dictionary to keep track of all previously set
 * attributes.
 */
class GerberAttributeWriter final {
  Q_DECLARE_TR_FUNCTIONS(GerberAttributeWriter)

public:
  // Constructors / Destructor
  GerberAttributeWriter() noexcept;
  GerberAttributeWriter(const GerberAttributeWriter& other) = delete;
  ~GerberAttributeWriter() noexcept;

  // General Methods
  QString setAttributes(const QList<GerberAttribute>& attributes) noexcept;

  // Operator Overloadings
  GerberAttributeWriter& operator=(const GerberAttributeWriter& rhs) = delete;

private:  // Data
  /// All currently set attributes, except file attributes
  QVector<GerberAttribute> mDictionary;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
