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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematicclipboard.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicClipboard::SchematicClipboard() noexcept
  : QObject(nullptr), mCutActive(false) {
}

SchematicClipboard::~SchematicClipboard() noexcept {
  // clear();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

/*void SchematicClipboard::clear() noexcept
{
    qDeleteAll(mSymbolInstances);       mSymbolInstances.clear();
}

void SchematicClipboard::cut(const QList<SymbolInstance*>& symbols)
{
    mCutActive = true;
    setElements(symbols);
}

void SchematicClipboard::copy(const QList<SymbolInstance*>& symbols)
{
    mCutActive = false;
    setElements(symbols);
}

void SchematicClipboard::paste(Schematic& schematic, QList<SymbolInstance*>&
symbols)
{
    Q_ASSERT(symbols.isEmpty() == true);

    foreach (DomElement* element, mSymbolInstances)
    {
        symbols.append(new SymbolInstance(schematic, *element));
    }

    mCutActive = false;
}*/

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

/*void SchematicClipboard::setElements(const QList<SymbolInstance*>& symbols)
{
    clear();

    foreach (SymbolInstance* symbol, symbols)
        mSymbolInstances.append(symbol->serializeToDomElement());
}*/

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
