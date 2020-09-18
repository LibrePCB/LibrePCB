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

#ifndef LIBREPCB_LIBRARY_CMDLIBRARYBASEELEMENTEDIT_H
#define LIBREPCB_LIBRARY_CMDLIBRARYBASEELEMENTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../librarybaseelement.h"

#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class CmdLibraryBaseElementEdit
 ******************************************************************************/

/**
 * @brief The CmdLibraryBaseElementEdit class
 */
class CmdLibraryBaseElementEdit : public UndoCommand {
public:
  // Constructors / Destructor
  CmdLibraryBaseElementEdit() = delete;
  CmdLibraryBaseElementEdit(const CmdLibraryBaseElementEdit& other) = delete;
  explicit CmdLibraryBaseElementEdit(LibraryBaseElement& element,
                                     const QString& text) noexcept;
  virtual ~CmdLibraryBaseElementEdit() noexcept;

  // Setters
  void setName(const QString& locale, const ElementName& name) noexcept;
  void setNames(const LocalizedNameMap& names) noexcept;
  void setDescription(const QString& locale, const QString& desc) noexcept;
  void setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept;
  void setKeywords(const QString& locale, const QString& keywords) noexcept;
  void setKeywords(const LocalizedKeywordsMap& keywords) noexcept;
  void setVersion(const Version& version) noexcept;
  void setAuthor(const QString& author) noexcept;
  void setDeprecated(bool deprecated) noexcept;

  // Operator Overloadings
  CmdLibraryBaseElementEdit& operator=(const CmdLibraryBaseElementEdit& rhs) =
      delete;

protected:  // Methods
  /// @copydoc UndoCommand::performExecute()
  virtual bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  virtual void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  virtual void performRedo() override;

private:  // Data
  LibraryBaseElement& mElement;

  LocalizedNameMap mOldNames;
  LocalizedNameMap mNewNames;
  LocalizedDescriptionMap mOldDescriptions;
  LocalizedDescriptionMap mNewDescriptions;
  LocalizedKeywordsMap mOldKeywords;
  LocalizedKeywordsMap mNewKeywords;
  Version mOldVersion;
  Version mNewVersion;
  QString mOldAuthor;
  QString mNewAuthor;
  bool mOldDeprecated;
  bool mNewDeprecated;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_CMDLIBRARYBASEELEMENTEDIT_H
