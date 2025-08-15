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

#ifndef LIBREPCB_EDITOR_CMDIMAGEADD_H
#define LIBREPCB_EDITOR_CMDIMAGEADD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"

#include <librepcb/core/geometry/image.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TransactionalDirectory;

namespace editor {

/*******************************************************************************
 *  Class CmdImageAdd
 ******************************************************************************/

/**
 * @brief The CmdImageAdd class
 */
class CmdImageAdd final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdImageAdd() = delete;
  CmdImageAdd(const CmdImageAdd& other) = delete;

  /**
   * @brief Constructor
   *
   * @param list        The list to add the image to.
   * @param dir         The directory to add the file to.
   * @param image       The image to add.
   * @param fileContent The file content to add. If NULL, no file is added,
   *                    only verified that the file exists already. If not
   *                    NULL, it is verified that the file does not exist yet.
   */
  CmdImageAdd(ImageList& list, TransactionalDirectory& dir,
              std::shared_ptr<Image> image,
              const QByteArray& fileContent) noexcept;
  ~CmdImageAdd() noexcept;

  // Operator Overloadings
  CmdImageAdd& operator=(const CmdImageAdd& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  ImageList& mList;
  TransactionalDirectory& mDirectory;
  std::shared_ptr<Image> mImage;
  const QByteArray mFileContent;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
