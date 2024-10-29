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

#ifndef LIBREPCB_EDITOR_MENUBUILDER_H
#define LIBREPCB_EDITOR_MENUBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class MenuBuilder
 ******************************************************************************/

/**
 * @brief Helper to easily create a QMenu
 */
class MenuBuilder final {
  Q_DECLARE_TR_FUNCTIONS(MenuBuilder)

public:
  // Types
  typedef QMenu* (*MenuFactory)(QWidget*);

  enum class Flag {
    DefaultAction = (1 << 0),  ///< Set action as default.
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  // Constructors / Destructor
  MenuBuilder() = delete;
  MenuBuilder(const MenuBuilder& other) = delete;
  explicit MenuBuilder(QMenuBar* menuBar) noexcept;
  explicit MenuBuilder(QMenu* menu) noexcept;
  ~MenuBuilder() noexcept;

  // General Methods
  QMenu* newMenu(MenuFactory factory) noexcept;
  QMenu* addSubMenu(MenuFactory factory) noexcept;
  QMenu* addSubMenu(const QString& objectName, const QString& title,
                    const QIcon& icon = QIcon()) noexcept;
  void addAction(QAction* action, Flags flags = Flags()) noexcept;
  void addAction(const QScopedPointer<QAction>& action,
                 Flags flags = Flags()) noexcept;
  void addSection(const QString& text, const QIcon& icon = QIcon()) noexcept;
  void addSeparator() noexcept;

  // Operator Overloadings
  MenuBuilder& operator=(const MenuBuilder& rhs) = delete;

  // Static Methods
  static QMenu* createFileMenu(QWidget* parent) noexcept;
  static QMenu* createEditMenu(QWidget* parent) noexcept;
  static QMenu* createViewMenu(QWidget* parent) noexcept;
  static QMenu* createSchematicMenu(QWidget* parent) noexcept;
  static QMenu* createBoardMenu(QWidget* parent) noexcept;
  static QMenu* createProjectMenu(QWidget* parent) noexcept;
  static QMenu* createToolsMenu(QWidget* parent) noexcept;
  static QMenu* createExtrasMenu(QWidget* parent) noexcept;
  static QMenu* createHelpMenu(QWidget* parent) noexcept;
  static QMenu* createGoToDockMenu(QWidget* parent) noexcept;
  static QMenu* createDocksVisibilityMenu(QWidget* parent) noexcept;
  static QMenu* createImportMenu(QWidget* parent) noexcept;
  static QMenu* createExportMenu(QWidget* parent) noexcept;
  static QMenu* createProductionDataMenu(QWidget* parent) noexcept;
  static QMenu* createLineWidthMenu(QWidget* parent) noexcept;
  static QMenu* createChangeDeviceMenu(QWidget* parent) noexcept;
  static QMenu* createChangeFootprintMenu(QWidget* parent) noexcept;
  static QMenu* createChangeModelMenu(QWidget* parent) noexcept;
  static QMenu* createMoveToOtherLibraryMenu(QWidget* parent) noexcept;
  static QMenu* createMoreResourcesMenu(QWidget* parent) noexcept;

private:  // Methods
  static QMenu* createMenu(const QString& objectName, const QString& text,
                           const QIcon& icon, QWidget* parent) noexcept;

private:  // Data
  QMenuBar* mMenuBar;
  QPointer<QMenu> mMenu;
};

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::editor::MenuBuilder::Flags)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
