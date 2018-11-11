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

#ifndef LIBREPCB_LIBRARY_CMDCOMPONENTEDIT_H
#define LIBREPCB_LIBRARY_CMDCOMPONENTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlibraryelementedit.h"
#include "../component.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class CmdComponentEdit
 ******************************************************************************/

/**
 * @brief The CmdComponentEdit class
 */
class CmdComponentEdit : public CmdLibraryElementEdit {
public:
  // Constructors / Destructor
  CmdComponentEdit()                              = delete;
  CmdComponentEdit(const CmdComponentEdit& other) = delete;
  explicit CmdComponentEdit(Component& component) noexcept;
  virtual ~CmdComponentEdit() noexcept;

  // Setters
  void setIsSchematicOnly(bool schematicOnly) noexcept;
  void setDefaultValue(const QString& value) noexcept;
  void setPrefix(const QString& norm, const ComponentPrefix& prefix) noexcept;
  void setPrefixes(const NormDependentPrefixMap& prefixes) noexcept;
  void setAttributes(const AttributeList& attributes) noexcept;

  // Operator Overloadings
  CmdComponentEdit& operator=(const CmdComponentEdit& rhs) = delete;

protected:  // Methods
  /// @copydoc UndoCommand::performExecute()
  virtual bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  virtual void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  virtual void performRedo() override;

private:  // Data
  Component& mComponent;

  bool                   mOldSchematicOnly;
  bool                   mNewSchematicOnly;
  QString                mOldDefaultValue;
  QString                mNewDefaultValue;
  NormDependentPrefixMap mOldPrefixes;
  NormDependentPrefixMap mNewPrefixes;
  AttributeList          mOldAttributes;
  AttributeList          mNewAttributes;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_CMDCOMPONENTEDIT_H
