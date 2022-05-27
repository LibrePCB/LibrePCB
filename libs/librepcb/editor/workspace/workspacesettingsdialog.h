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

#ifndef LIBREPCB_EDITOR_WORKSPACESETTINGSDIALOG_H
#define LIBREPCB_EDITOR_WORKSPACESETTINGSDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../modelview/editablelistmodel.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;
class WorkspaceSettings;

namespace editor {

namespace Ui {
class WorkspaceSettingsDialog;
}

/*******************************************************************************
 *  Class WorkspaceSettingsDialog
 ******************************************************************************/

/**
 * @brief Dialog (GUI) to view and modify workspace settings
 */
class WorkspaceSettingsDialog final : public QDialog {
  Q_OBJECT

  using LibraryLocaleOrderModel =
      EditableListModel<QStringList, EditableListModelType::LOCALE>;
  using LibraryNormOrderModel = EditableListModel<QStringList>;
  using RepositoryUrlModel = EditableListModel<QList<QUrl>>;

public:
  // Constructors / Destructor
  WorkspaceSettingsDialog() = delete;
  WorkspaceSettingsDialog(const WorkspaceSettingsDialog& other) = delete;
  explicit WorkspaceSettingsDialog(Workspace& workspace,
                                   QWidget* parent = nullptr);
  ~WorkspaceSettingsDialog();

  // Operator Overloadings
  WorkspaceSettingsDialog& operator=(const WorkspaceSettingsDialog& rhs) =
      delete;

private:
  void buttonBoxClicked(QAbstractButton* button) noexcept;
  void loadSettings() noexcept;
  void saveSettings() noexcept;

private:
  Workspace& mWorkspace;  /// Reference to the Workspace object
  WorkspaceSettings& mSettings;  ///< Reference to the WorkspaceSettings object
  QScopedPointer<LibraryLocaleOrderModel> mLibLocaleOrderModel;
  QScopedPointer<LibraryNormOrderModel> mLibNormOrderModel;
  QScopedPointer<RepositoryUrlModel> mRepositoryUrlsModel;
  QScopedPointer<Ui::WorkspaceSettingsDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
