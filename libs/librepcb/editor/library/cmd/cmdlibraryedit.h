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

#ifndef LIBREPCB_EDITOR_CMDLIBRARYEDIT_H
#define LIBREPCB_EDITOR_CMDLIBRARYEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdlibrarybaseelementedit.h"

#include <librepcb/core/library/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdLibraryEdit
 ******************************************************************************/

/**
 * @brief The CmdLibraryEdit class
 */
class CmdLibraryEdit : public CmdLibraryBaseElementEdit {
public:
  // Constructors / Destructor
  CmdLibraryEdit() = delete;
  CmdLibraryEdit(const CmdLibraryEdit& other) = delete;
  explicit CmdLibraryEdit(Library& library) noexcept;
  virtual ~CmdLibraryEdit() noexcept;

  // Setters
  void setUrl(const QUrl& url) noexcept;
  void setDependencies(const QSet<Uuid>& deps) noexcept;
  void setIcon(const QByteArray& png) noexcept;
  void setManufacturer(const SimpleString& value) noexcept;

  // Operator Overloadings
  CmdLibraryEdit& operator=(const CmdLibraryEdit& rhs) = delete;

protected:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  virtual bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  virtual void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  virtual void performRedo() override;

private:  // Data
  Library& mLibrary;

  QUrl mOldUrl;
  QUrl mNewUrl;
  QSet<Uuid> mOldDependencies;
  QSet<Uuid> mNewDependencies;
  QByteArray mOldIcon;
  QByteArray mNewIcon;
  SimpleString mOldManufacturer;
  SimpleString mNewManufacturer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
