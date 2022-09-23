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

#ifndef LIBREPCB_EDITOR_TOOLBARPROXY_H
#define LIBREPCB_EDITOR_TOOLBARPROXY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class ToolBarProxy
 ******************************************************************************/

/**
 * @brief The ToolBarProxy class allows to map a list of QAction's to one
 * QToolBar
 */
class ToolBarProxy final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ToolBarProxy(QObject* parent = nullptr) noexcept;
  ToolBarProxy(const ToolBarProxy& other) = delete;
  ~ToolBarProxy() noexcept;

  // Setters
  void setToolBar(QToolBar* toolbar) noexcept;
  void setEnabled(bool enabled) noexcept;

  // General Methods
  void clear() noexcept;
  QAction* addAction(std::unique_ptr<QAction> action) noexcept;
  void addActionGroup(std::unique_ptr<QActionGroup> group) noexcept;
  QAction* addLabel(const QString& text, int indent = 0) noexcept;
  QAction* addWidget(std::unique_ptr<QWidget> widget, int indent = 0) noexcept;
  QAction* addSeparator() noexcept;
  void removeAction(QAction* action) noexcept;
  bool startTabFocusCycle(QWidget& returnFocusWidget);

  // Operator Overloadings
  ToolBarProxy& operator=(const ToolBarProxy& rhs) = delete;

private:  // Data
  QToolBar* mToolBar;
  QList<QAction*> mActions;
  QList<QActionGroup*> mActionGroups;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
