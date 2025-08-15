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

#ifndef LIBREPCB_EDITOR_CMDSCHEMATICIMAGEADD_H
#define LIBREPCB_EDITOR_CMDSCHEMATICIMAGEADD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SI_Image;
class Schematic;
class TransactionalDirectory;

namespace editor {

/*******************************************************************************
 *  Class CmdSchematicImageAdd
 ******************************************************************************/

/**
 * @brief The CmdSchematicImageAdd class
 */
class CmdSchematicImageAdd final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdSchematicImageAdd() = delete;
  CmdSchematicImageAdd(const CmdSchematicImageAdd& other) = delete;

  /**
   * @brief Constructor
   *
   * @param image       The image to add.
   * @param dir         The directory to add the file to.
   * @param fileContent The file content to add. If NULL, no file is added,
   *                    only verified that the file exists already. If not
   *                    NULL, it is verified that the file does not exist yet.
   */
  explicit CmdSchematicImageAdd(SI_Image& image, TransactionalDirectory& dir,
                                const QByteArray& fileContent) noexcept;
  ~CmdSchematicImageAdd() noexcept;

  // Operator Overloadings
  CmdSchematicImageAdd& operator=(const CmdSchematicImageAdd& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  SI_Image& mImage;
  TransactionalDirectory& mDirectory;
  const QByteArray mFileContent;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
