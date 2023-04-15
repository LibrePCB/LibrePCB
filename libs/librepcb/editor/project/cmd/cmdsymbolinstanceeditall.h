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

#ifndef LIBREPCB_EDITOR_CMDSYMBOLINSTANCEEDITALL_H
#define LIBREPCB_EDITOR_CMDSYMBOLINSTANCEEDITALL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Point;
class SI_Symbol;

namespace editor {

class CmdSymbolInstanceEdit;
class CmdTextEdit;

/*******************************************************************************
 *  Class CmdSymbolInstanceEditAll
 ******************************************************************************/

/**
 * @brief The CmdSymbolInstanceEditAll class
 */
class CmdSymbolInstanceEditAll final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  explicit CmdSymbolInstanceEditAll(SI_Symbol& symbol) noexcept;
  ~CmdSymbolInstanceEditAll() noexcept;

  // General Methods
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void setMirrored(bool mirrored, bool immediate) noexcept;
  void mirror(const Point& center, Qt::Orientation orientation,
              bool immediate) noexcept;

private:
  CmdSymbolInstanceEdit* mSymEditCmd;
  QVector<CmdTextEdit*> mTextEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
