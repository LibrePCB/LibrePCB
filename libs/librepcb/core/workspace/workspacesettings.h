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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGS_H
#define LIBREPCB_CORE_WORKSPACESETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../types/lengthunit.h"
#include "workspacesettingsitem_genericvalue.h"
#include "workspacesettingsitem_genericvaluelist.h"
#include "workspacesettingsitem_keyboardshortcuts.h"
#include "workspacesettingsitem_themes.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;
class Version;

/*******************************************************************************
 *  Class WorkspaceSettings
 ******************************************************************************/

/**
 * @brief Container for all workspace related settings
 *
 * The "settings.lp" file in a workspace is used to store workspace related
 * settings. This class is an interface to those settings. A
 * ::librepcb::WorkspaceSettings object is created in the
 * constructor of the ::librepcb::Workspace object.
 *
 * Each settings item is represented by an instance of a
 * ::librepcb::WorkspaceSettingsItem subclass.
 *
 * @see ::librepcb::WorkspaceSettingsItem
 */
class WorkspaceSettings final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  WorkspaceSettings(const WorkspaceSettings& other) = delete;
  explicit WorkspaceSettings(QObject* parent = nullptr);
  ~WorkspaceSettings() noexcept;

  /**
   * @brief Load settings from file
   *
   * @param node        S-Expression node of settings file.
   * @param fileFormat  File format of settings file.
   */
  void load(const SExpression& node, const Version& fileFormat);

  /**
   * @brief Reset all settings to their default value
   */
  void restoreDefaults() noexcept;

  /**
   * @brief Serialize settings to ::librepcb::SExpression
   *
   * @return ::librepcb::SExpression node containing all settings.
   */
  SExpression serialize();

  // Operator Overloadings
  WorkspaceSettings& operator=(const WorkspaceSettings& rhs) = delete;

private:  // Methods
  /**
   * @brief Get all ::librepcb::WorkspaceSettingsItem objects
   *
   * @return List of ::librepcb::WorkspaceSettingsItem objects
   */
  QList<WorkspaceSettingsItem*> getAllItems() const noexcept;

private:  // Data
  /**
   * @brief Settings nodes loaded by #load()
   *
   * This map is filled with all settings S-Expression nodes when loading the
   * settings from file. When modifying settings with the workspace settings
   * dialog, the nodes in this map are updated accordingly. When saving the
   * settings to file, these S-Expression nodes will be written to the file.
   *
   * - Key: Settings key, e.g. "use_opengl"
   * - Value: The corresponding serialization, e.g. "(use_opengl true)"
   *
   * Important:
   *
   *   - Keeping unknown settings is important to not loose them when opening
   *     a workspace after an application downgrade.
   *   - When restoring default settings, the corresponding (or all) entries
   *     are removed from this map (i.e. not written to file at all). This
   *     ensures that users will automatically profit from improved default
   *     values after an application upgrade unless they have manually changed
   *     them.
   *   - QMap is sorted by key, which will lead to sorted entries in the
   *     S-Expression file for a clean file format.
   */
  QMap<QString, SExpression> mFileContent;

  /**
   * @brief Whether #mFileContent needs to be upgraded or not
   */
  bool mUpgradeRequired;

public:
  // All settings item objects below. The order is not relevant for saving,
  // it should be ordered logically.
  //
  // Note: Generally we don't make member variables public, but in this case
  //       it would create a lot of boilerplate to wrap all objects with
  //       both const- and non-const methods, and it's totally safe to access
  //       them directly.

  /**
   * @brief User name
   *
   * Used when creating new library elements or projects.
   *
   * Default: "" (but gets initialized when creating a new workspace)
   */
  WorkspaceSettingsItem_GenericValue<QString> userName;

  /**
   * @brief The application's locale (e.g. "en_US")
   *
   * An empty string means that the system locale will be used.
   *
   * Default: ""
   */
  WorkspaceSettingsItem_GenericValue<QString> applicationLocale;

  /**
   * @brief The application's default length unit
   *
   * Default: millimeters
   */
  WorkspaceSettingsItem_GenericValue<LengthUnit> defaultLengthUnit;

  /**
   * @brief Project autosave interval [seconds] (0 = autosave disabled)
   *
   * Default: 600
   */
  WorkspaceSettingsItem_GenericValue<uint> projectAutosaveIntervalSeconds;

  /**
   * @brief Use OpenGL hardware acceleration
   *
   * Default: False
   */
  WorkspaceSettingsItem_GenericValue<bool> useOpenGl;

  /**
   * @brief Preferred library locales (like "de_CH") in the right order
   *
   * The locale which should be used first is at index 0 of the list. If no
   * translation strings are found for all locales in this list, the fallback
   * locale "en_US" will be used automatically, so the list do not have to
   * contain "en_US". An empty list is also valid, then the fallback locale
   * "en_US" will be used.
   *
   * Default: []
   */
  WorkspaceSettingsItem_GenericValueList<QStringList> libraryLocaleOrder;

  /**
   * @brief Preferred library norms (like "DIN EN 81346") in the right order
   *
   * The norm which should be used first is at index 0 of the list.
   *
   * Default: []
   */
  WorkspaceSettingsItem_GenericValueList<QStringList> libraryNormOrder;

  /**
   * @brief The list of API endpoint URLs in the right order
   *
   * The endpoint with the highest priority is at index 0 of the list. In case
   * of version conflicts, the endpoint with the higher priority will be used.
   *
   * Default: ["https://api.librepcb.org"]
   */
  WorkspaceSettingsItem_GenericValueList<QList<QUrl>> apiEndpoints;

  /**
   * @brief Enable auto-fetch of live parts information (through #apiEndpoints)
   *
   * Default: True
   */
  WorkspaceSettingsItem_GenericValue<bool> autofetchLivePartInformation;

  /**
   * @brief Custom command(s) to be used for opening web URLs
   *
   * When opening an URL, the application will iterate through this list of
   * commands until a command was successful. If none was successful, the
   * system's default command will be used as fallback.
   *
   * Supported placeholders: `{{URL}}`
   *
   * Default: []
   */
  WorkspaceSettingsItem_GenericValueList<QStringList>
      externalWebBrowserCommands;

  /**
   * @brief Custom command(s) to be used for opening directories
   *
   * When opening a directory, the application will iterate through this list
   * of commands until a command was successful. If none was successful, the
   * system's default command will be used as fallback.
   *
   * Supported placeholders: `{{URL}}`, `{{FILEPATH}}`
   *
   * Default: []
   */
  WorkspaceSettingsItem_GenericValueList<QStringList>
      externalFileManagerCommands;

  /**
   * @brief Custom command(s) to be used for opening PDF files
   *
   * When opening a PDF file, the application will iterate through this list
   * of commands until a command was successful. If none was successful, the
   * system's default command will be used as fallback.
   *
   * Supported placeholders: `{{URL}}`, `{{FILEPATH}}`
   *
   * Default: []
   */
  WorkspaceSettingsItem_GenericValueList<QStringList> externalPdfReaderCommands;

  /**
   * @brief Keyboard shortcuts
   *
   * @note Expected to contain only the shortcuts explicitly set (overridden)
   *       by the user, not all available shortcuts. This way we are able to
   *       improve the default shortcuts with each new release without users
   *       staying at the old shortcuts.
   *
   * @see ::librepcb::WorkspaceSettingsItem_KeyboardShortcuts
   */
  WorkspaceSettingsItem_KeyboardShortcuts keyboardShortcuts;

  /**
   * @brief Themes
   *
   * @see ::librepcb::WorkspaceSettingsItem_KeyboardShortcuts
   */
  WorkspaceSettingsItem_Themes themes;

  /**
   * @brief Dismissed messages
   *
   * List of messages which the user dismissed with "do not show again". It's
   * just a generic list of strings, where each message is identified by some
   * locale-independent string. It's recommended to use UPPER_SNAKE_CASE
   * strings, For example: "WORKSPACE_V0.1_HAS_NO_LIBRARIES".
   *
   * Default: []
   */
  WorkspaceSettingsItem_GenericValueList<QSet<QString>> dismissedMessages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
