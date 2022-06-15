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

#ifndef LIBREPCB_EDITOR_STANDARDEDITORCOMMANDHANDLER_H
#define LIBREPCB_EDITOR_STANDARDEDITORCOMMANDHANDLER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class WorkspaceSettings;

namespace editor {

/*******************************************************************************
 *  Class StandardEditorCommandHandler
 ******************************************************************************/

/**
 * @brief Helper to handle some of the ::librepcb::editor::EditorCommand actions
 *
 * Indended to share code between the various editors.
 */
class StandardEditorCommandHandler final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  StandardEditorCommandHandler() = delete;
  StandardEditorCommandHandler(const StandardEditorCommandHandler& other) =
      delete;
  StandardEditorCommandHandler(const WorkspaceSettings& settings,
                               QWidget* parent = nullptr) noexcept;
  ~StandardEditorCommandHandler() noexcept;

  // Action Handlers
  void aboutLibrePcb() const noexcept;
  void onlineDocumentation() const noexcept;
  void website() const noexcept;
  void fileManager(const FilePath& fp) const noexcept;

  // Operator Overloadings
  StandardEditorCommandHandler& operator=(
      const StandardEditorCommandHandler& rhs) = delete;

private:  // Data
  const WorkspaceSettings& mSettings;
  QPointer<QWidget> mParent;
};

}  // namespace editor
}  // namespace librepcb

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
