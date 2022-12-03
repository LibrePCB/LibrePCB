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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class WorkspaceSettingsItem
 ******************************************************************************/

/**
 * @brief Base class for all workspace settings items
 *
 * For simple settings, see
 * ::librepcb::WorkspaceSettingsItem_GenericValue and
 * ::librepcb::WorkspaceSettingsItem_GenericValueList.
 *
 * @see ::librepcb::WorkspaceSettingsItem_GenericValue
 * @see ::librepcb::WorkspaceSettingsItem_GenericValueList
 */
class WorkspaceSettingsItem : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  WorkspaceSettingsItem() = delete;
  explicit WorkspaceSettingsItem(const QString& key,
                                 QObject* parent = nullptr) noexcept;
  WorkspaceSettingsItem(const WorkspaceSettingsItem& other) = delete;
  virtual ~WorkspaceSettingsItem() noexcept;

  /**
   * @brief Get the setting key used for serialization
   *
   * @return Serialization key.
   */
  const QString& getKey() const noexcept { return mKey; }

  /**
   * @brief Check whether this setting is at its default value (not modified)
   *
   * @retval true   Default is active, value is not stored in settings file.
   * @retval false  Value has been modified and is stored in settings file.
   */
  bool isDefaultValue() const noexcept { return mIsDefault; }

  /**
   * @brief Check whether this setting was edited sinc the last load or save
   *
   * @retval true   Value has been modified.
   * @retval false  Value not modified, settings file content is still valid.
   */
  bool isEdited() const noexcept { return mEdited; }

  /**
   * @brief Restore default value
   */
  void restoreDefault() noexcept;

  /**
   * @brief Load value from S-Expression file
   *
   * @param root        Loaded ::librepcb::SExpression node.
   */
  void load(const SExpression& root);

  /**
   * @brief Serialize the value into S-Expression nodes
   *
   * @param root  S-Expression node to be updated.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  WorkspaceSettingsItem& operator=(const WorkspaceSettingsItem& rhs) = delete;

signals:
  /**
   * @brief Signal to notify about changes of the settings value
   */
  void edited();

protected:
  void valueModified() noexcept;

  /**
   * @brief Restore default value
   *
   * @note Implementation must emit the #edited() signal if the value has
   *       changed.
   */
  virtual void restoreDefaultImpl() noexcept = 0;

  /**
   * @brief Load value from S-Expression node
   *
   * @param root        S-Expression node of the settings element.
   *
   * @note Implementation must emit the #edited() signal if the value has
   *       changed.
   *
   * @note Implementation must be atomic, i.e. either the value must be loaded
   *       completely from file, or left at the old value (in case of errors).
   */
  virtual void loadImpl(const SExpression& root) = 0;

  /**
   * @brief Serialize the value into S-Expression node
   *
   * @param root  S-Expression node to be updated.
   */
  virtual void serializeImpl(SExpression& root) const = 0;

private:
  QString mKey;  ///< Key used for serialization
  bool mIsDefault;  ///< Setting is at default value
  mutable bool mEdited;  ///< Edited since last load or save
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
