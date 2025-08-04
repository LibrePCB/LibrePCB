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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "menubuilder.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MenuBuilder::MenuBuilder(QMenuBar* menuBar) noexcept
  : mMenuBar(menuBar), mMenu(nullptr) {
}

MenuBuilder::MenuBuilder(QMenu* menu) noexcept
  : mMenuBar(nullptr), mMenu(menu) {
}

MenuBuilder::~MenuBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QMenu* MenuBuilder::newMenu(MenuFactory factory) noexcept {
  if (mMenuBar && factory) {
    mMenu = factory(mMenuBar);
    mMenuBar->addMenu(mMenu);
  }
  return mMenu;
}

QMenu* MenuBuilder::addSubMenu(MenuFactory factory) noexcept {
  QMenu* submenu = nullptr;
  if (mMenu && factory) {
    submenu = factory(mMenu);
    mMenu->addMenu(submenu);
  }
  return submenu;
}

QMenu* MenuBuilder::addSubMenu(const QString& objectName, const QString& title,
                               const QIcon& icon) noexcept {
  QMenu* submenu = nullptr;
  if (mMenu) {
    submenu = mMenu->addMenu(icon, title);
    submenu->setObjectName(objectName);
  }
  return submenu;
}

void MenuBuilder::addAction(QAction* action, Flags flags) noexcept {
  if (mMenu && action) {
    mMenu->addAction(action);
    if (flags.testFlag(Flag::DefaultAction)) {
      mMenu->setDefaultAction(action);
    }
  }
}

void MenuBuilder::addAction(const QScopedPointer<QAction>& action,
                            Flags flags) noexcept {
  addAction(action.data(), flags);
}

void MenuBuilder::addSection(const QString& text, const QIcon& icon) noexcept {
  if (mMenu) {
    mMenu->addSection(icon, text);
  }
}

void MenuBuilder::addSeparator() noexcept {
  if (mMenu) {
    mMenu->addSeparator();
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QMenu* MenuBuilder::createChangeDeviceMenu(QWidget* parent) noexcept {
  return createMenu("menuChangeDevice", tr("Change &Device"),
                    QIcon(":/img/library/package.png"), parent);
}

QMenu* MenuBuilder::createChangeFootprintMenu(QWidget* parent) noexcept {
  return createMenu("menuChangeFootprint", tr("Change &Footprint"),
                    QIcon(":/img/library/footprint.png"), parent);
}

QMenu* MenuBuilder::createChangeModelMenu(QWidget* parent) noexcept {
  return createMenu("menuChangeModel", tr("Change 3D &Model"),
                    QIcon(":/img/library/3d_model.png"), parent);
}

QMenu* MenuBuilder::createMoreResourcesMenu(QWidget* parent) noexcept {
  return createMenu("menuMoreResources", tr("More Resources"), QIcon(), parent);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QMenu* MenuBuilder::createMenu(const QString& objectName, const QString& text,
                               const QIcon& icon, QWidget* parent) noexcept {
  QMenu* menu = new QMenu(text, parent);
  menu->setObjectName(objectName);
  menu->setIcon(icon);
  return menu;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
