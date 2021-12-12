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

#ifndef LIBREPCB_EDITOR_CMDCOMPONENTEDIT_H
#define LIBREPCB_EDITOR_CMDCOMPONENTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdlibraryelementedit.h"

#include <librepcb/core/library/cmp/component.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdComponentEdit
 ******************************************************************************/

/**
 * @brief The CmdComponentEdit class
 */
class CmdComponentEdit : public CmdLibraryElementEdit {
public:
  // Constructors / Destructor
  CmdComponentEdit() = delete;
  CmdComponentEdit(const CmdComponentEdit& other) = delete;
  explicit CmdComponentEdit(Component& component) noexcept;
  virtual ~CmdComponentEdit() noexcept;

  // Setters
  void setIsSchematicOnly(bool schematicOnly) noexcept;
  void setDefaultValue(const QString& value) noexcept;
  void setPrefix(const QString& norm, const ComponentPrefix& prefix) noexcept;
  void setPrefixes(const NormDependentPrefixMap& prefixes) noexcept;

  // Operator Overloadings
  CmdComponentEdit& operator=(const CmdComponentEdit& rhs) = delete;

protected:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  virtual bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  virtual void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  virtual void performRedo() override;

private:  // Data
  Component& mComponent;

  bool mOldSchematicOnly;
  bool mNewSchematicOnly;
  QString mOldDefaultValue;
  QString mNewDefaultValue;
  NormDependentPrefixMap mOldPrefixes;
  NormDependentPrefixMap mNewPrefixes;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
