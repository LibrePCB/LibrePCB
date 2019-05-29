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

#ifndef LIBREPCB_PROJECT_SCHEMATICCLIPBOARD_H
#define LIBREPCB_PROJECT_SCHEMATICCLIPBOARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class DomElement;

namespace project {

class Schematic;

namespace editor {

/*******************************************************************************
 *  Class SchematicClipboard
 ******************************************************************************/

/**
 * @brief The SchematicClipboard class
 */
class SchematicClipboard final : public QObject {
  Q_OBJECT

public:
  // General Methods
  // void clear() noexcept;
  // void cut(const QList<SymbolInstance*>& symbols);
  // void copy(const QList<SymbolInstance*>& symbols);
  // void paste(Schematic& schematic, QList<SymbolInstance*>& symbols);

  // Static Methods
  static SchematicClipboard& instance() noexcept {
    static SchematicClipboard i;
    return i;
  }

private:
  // Private Methods
  SchematicClipboard() noexcept;
  ~SchematicClipboard() noexcept;
  // void setElements(const QList<SymbolInstance*>& symbols);

  // Attributes
  bool mCutActive;
  // QList<DomElement*> mSymbolInstances;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SCHEMATICCLIPBOARD_H
