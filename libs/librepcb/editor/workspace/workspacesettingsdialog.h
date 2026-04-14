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
#include "ui.h"

#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/core/workspace/workspacesettingsitem_genericvaluelist.h>

#include <QtCore>
#include <QtWidgets>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;
class WorkspaceSettings;
struct UiTheme;

namespace editor {

class ApiEndpointListModelLegacy;
class KeyboardShortcutsModel;

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

  struct ExternalApplication {
    QPointer<WorkspaceSettingsItem_GenericValueList<QStringList>> setting;
    QString exampleExecutable;
    QString defaultArgument;
    QVector<std::pair<QString, QString>> placeholders;
    QStringList currentValue;
  };

public:
  // Constructors / Destructor
  WorkspaceSettingsDialog() = delete;
  WorkspaceSettingsDialog(const WorkspaceSettingsDialog& other) = delete;
  explicit WorkspaceSettingsDialog(Workspace& workspace, const UiTheme& theme,
                                   QWidget* parent = nullptr);
  ~WorkspaceSettingsDialog();

  // Operator Overloadings
  WorkspaceSettingsDialog& operator=(const WorkspaceSettingsDialog& rhs) =
      delete;

signals:
  void desktopIntegrationStatusChanged();

private:
  void buttonBoxClicked(QAbstractButton* button) noexcept;
  void keyPressEvent(QKeyEvent* event) noexcept override;
  void changeEvent(QEvent* event) noexcept override;
  void reject() noexcept override;
  void externalApplicationListIndexChanged(int index) noexcept;
  void updateColorSchemes() noexcept;
  void execColorSchemeDialog(std::shared_ptr<UserColorScheme> scheme,
                             bool focusNameEdt = false) noexcept;
  void updateDismissedMessagesCount() noexcept;
  void updateDesktopIntegrationStatus() noexcept;
  void loadSettings() noexcept;
  void saveSettings() noexcept;
  void discardTemporaryModifications() noexcept;
  void revertTemporaryModifications() noexcept;
  bool hasTemporaryModifications() const noexcept;

private:
  Workspace& mWorkspace;  /// Reference to the Workspace object
  WorkspaceSettings& mSettings;  ///< Reference to the WorkspaceSettings object
  const UiTheme& mTheme;  ///< Reference to current UI theme
  QScopedPointer<LibraryLocaleOrderModel> mLibLocaleOrderModel;
  QScopedPointer<LibraryNormOrderModel> mLibNormOrderModel;
  QScopedPointer<ApiEndpointListModelLegacy> mApiEndpointModel;
  QScopedPointer<KeyboardShortcutsModel> mKeyboardShortcutsModel;
  QScopedPointer<QSortFilterProxyModel> mKeyboardShortcutsFilterModel;
  QScopedPointer<Ui::WorkspaceSettingsDialog> mUi;
  std::optional<slint::ComponentHandle<ui::ColorSchemeDialog>>
      mColorSchemeDialog;

  // Cached settings
  QVector<ExternalApplication> mExternalApplications;

  // Original values of temporarily changed settings
  QString mOldUiTheme;
  QString mOldApplicationLocale;
  GridStyle mOldSchematicGridStyle;
  GridStyle mOldBoardGridStyle;
  QMap<Uuid, UserColorScheme> mOldSchColorSchemes;
  QMap<Uuid, UserColorScheme> mOldBrdColorSchemes;
  QMap<Uuid, UserColorScheme> mOld3dColorSchemes;
  Uuid mOldSchColorSchemeActive;
  Uuid mOldBrdColorSchemeActive;
  Uuid mOld3dColorSchemeActive;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
