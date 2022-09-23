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
#include "toolbarproxy.h"

#include "editortoolbox.h"

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

ToolBarProxy::ToolBarProxy(QObject* parent) noexcept
  : QObject(parent), mToolBar(nullptr) {
}

ToolBarProxy::~ToolBarProxy() noexcept {
  clear();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ToolBarProxy::setToolBar(QToolBar* toolbar) noexcept {
  if (toolbar == mToolBar) {
    return;
  }

  if (mToolBar) {
    foreach (QAction* action, mActions) {
      Q_ASSERT(action);
      mToolBar->removeAction(action);
    }
  }

  mToolBar = toolbar;

  if (mToolBar) {
    foreach (QAction* action, mActions) {
      Q_ASSERT(action);
      mToolBar->addAction(action);
    }
  }
}

void ToolBarProxy::setEnabled(bool enabled) noexcept {
  foreach (QAction* action, mActions) {
    Q_ASSERT(action);
    action->setEnabled(enabled);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ToolBarProxy::clear() noexcept {
  foreach (QAction* action, mActions) {
    Q_ASSERT(action);
    removeAction(action);
  }
}

QAction* ToolBarProxy::addAction(std::unique_ptr<QAction> action) noexcept {
  Q_ASSERT(action);
  Q_ASSERT(!mActions.contains(action.get()));
  Q_ASSERT((action->parent() == nullptr) || (action->parent() == this));

  if (mToolBar) {
    mToolBar->addAction(action.get());
  }

  mActions.append(action.get());
  return action.release();
}

void ToolBarProxy::addActionGroup(
    std::unique_ptr<QActionGroup> group) noexcept {
  Q_ASSERT(group);
  Q_ASSERT(!mActionGroups.contains(group.get()));
  Q_ASSERT((group->parent() == nullptr) || (group->parent() == this));
  group->setParent(this);
  if (mToolBar) {
    mToolBar->addActions(group->actions());
  }
  mActions.append(group->actions());
  mActionGroups.append(group.release());
}

QAction* ToolBarProxy::addLabel(const QString& text, int indent) noexcept {
  std::unique_ptr<QLabel> label(new QLabel(text));
  label->setIndent(indent);
  return addWidget(std::move(label));
}

QAction* ToolBarProxy::addWidget(std::unique_ptr<QWidget> widget,
                                 int indent) noexcept {
  Q_ASSERT(widget);
  Q_ASSERT((widget->parent() == nullptr) || (widget->parent() == this));

  if (indent > 0) {
    addLabel("", indent);  // A bit ugly, but simple and works :)
  }

  std::unique_ptr<QWidgetAction> action(new QWidgetAction(this));
  action->setDefaultWidget(widget.release());  // transfer ownership to action
  return addAction(std::move(action));
}

QAction* ToolBarProxy::addSeparator() noexcept {
  std::unique_ptr<QAction> action(new QAction(this));
  action->setSeparator(true);
  return addAction(std::move(action));
}

void ToolBarProxy::removeAction(QAction* action) noexcept {
  Q_ASSERT(action);
  Q_ASSERT(mActions.contains(action));

  if (mToolBar) {
    mToolBar->removeAction(action);
  }

  mActions.removeOne(action);
  delete action;
}

bool ToolBarProxy::startTabFocusCycle(QWidget& returnFocusWidget) {
  return mToolBar &&
      EditorToolbox::startToolBarTabFocusCycle(*mToolBar, returnFocusWidget);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
