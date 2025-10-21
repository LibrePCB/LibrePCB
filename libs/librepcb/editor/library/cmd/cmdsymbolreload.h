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

#ifndef LIBREPCB_EDITOR_CMDSYMBOLRELOAD_H
#define LIBREPCB_EDITOR_CMDSYMBOLRELOAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdlibraryelementedit.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/sym/symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;

namespace editor {

/*******************************************************************************
 *  Class CmdSymbolReload
 ******************************************************************************/

/**
 * @brief The CmdSymbolReload class
 */
class CmdSymbolReload final : public CmdLibraryElementEdit {
public:
  // Constructors / Destructor
  CmdSymbolReload() = delete;
  CmdSymbolReload(const CmdSymbolReload& other) = delete;
  explicit CmdSymbolReload(Symbol& element) noexcept;
  virtual ~CmdSymbolReload() noexcept;

  // Operator Overloadings
  CmdSymbolReload& operator=(const CmdSymbolReload& rhs) = delete;

protected:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  virtual bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  virtual void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  virtual void performRedo() override;

private:  // Data
  Symbol& mElement;

  TransactionalFileSystem::State mOldFiles;
  TransactionalFileSystem::State mNewFiles;

  SymbolPinList mOldPins;
  SymbolPinList mNewPins;
  PolygonList mOldPolygons;
  PolygonList mNewPolygons;
  CircleList mOldCircles;
  CircleList mNewCircles;
  TextList mOldTexts;
  TextList mNewTexts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
