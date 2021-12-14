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
#include "../fileio/filepath.h"
#include "../serialization/serializableobject.h"
#include "../types/lengthunit.h"
#include "workspacesettingsitem_genericvalue.h"
#include "workspacesettingsitem_genericvaluelist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

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
class WorkspaceSettings final : public QObject, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  WorkspaceSettings() = delete;
  WorkspaceSettings(const WorkspaceSettings& other) = delete;
  explicit WorkspaceSettings(const FilePath& fp, const Version& fileFormat,
                             QObject* parent = nullptr);
  ~WorkspaceSettings() noexcept;

  /**
   * @brief Reset all settings to their default value
   */
  void restoreDefaults() noexcept;

  /**
   * @brief Save all settings to the file
   */
  void saveToFile() const;

  // Operator Overloadings
  WorkspaceSettings& operator=(const WorkspaceSettings& rhs) = delete;

private:  // Methods
  /**
   * @brief Get all ::librepcb::WorkspaceSettingsItem objects
   *
   * @return List of ::librepcb::WorkspaceSettingsItem objects
   */
  QList<WorkspaceSettingsItem*> getAllItems() const noexcept;

  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

private:  // Data
  FilePath mFilePath;  ///< path to the "settings.lp" file

public:
  // All settings item objects below, in the same order as they are safed in
  // the settings file.
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
   * @brief The list of API repository URLs in the right order
   *
   * The repository with the highest priority is at index 0 of the list. In case
   * of version conflicts, the repository with the higher priority will be used.
   *
   * Default: ["https://api.librepcb.org"]
   */
  WorkspaceSettingsItem_GenericValueList<QList<QUrl>> repositoryUrls;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
